#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <math.h>
#include <stdarg.h>
#include <wchar.h>

#include "tesmio_api.h"

static const TsmHost* H = NULL;
static unsigned char* g_base = NULL;
static const char* ENGINE_DLL = "C3DDLL64.dll";
// SOVIET64.exe 1.1.1.9: the live world object is static storage, not a
// pointer-sized global. Calendar Synchronizer 2.1 reads and writes this same
// object in its verified weather hook.
static const uintptr_t G_WORLD_OBJECT = 0x9D4F10;
static const char* PANEL_COLLISION =
    "?Collision@C3D_PANEL2D@@QEAA_NVC3DVECTOR3@@MM@Z";
static const char* PANEL_DRAW = "?Draw@C3D_PANEL2D@@QEAAXMMMMM_N@Z";
static const char* PANEL_9PATCH =
    "?DrawFrom9Patch@C3D_PANEL2D@@QEAAXUC3DRECT@@MMMMMM@Z";
static const char* PANEL_NEW_9PATCH =
    "?DrawFromNew9Patch@C3D_PANEL2D@@QEAAXUC3DRECT@@MMMMMM@Z";
static const char* PRINT_LEFT =
    "?PrintLeftUnicode@C3D_FONTMANAGER@@QEAAXPEAVC3D_FONT@@MMKPEB_WZZ";
static const char* PRINT_CENTER =
    "?PrintCenterUnicode@C3D_FONTMANAGER@@QEAAXPEAVC3D_FONT@@MMKPEB_WZZ";
static const char* PRINT_RIGHT =
    "?PrintRightUnicode@C3D_FONTMANAGER@@QEAAXPEAVC3D_FONT@@MMKPEB_WZZ";
static const char* FONT_PRINT_LEFT =
    "?PrintLeftUnicode@C3D_FONT@@QEAAXMMKPEB_WZZ";
static const char* FONT_PRINT_RIGHT =
    "?PrintRightUnicode@C3D_FONT@@QEAAXMMKPEB_WZZ";

typedef bool (*PanelCollisionFn)(void*, void*, float, float);
typedef void (*PanelDrawFn)(void*, float, float, float, float, float, bool);
struct C3DRect { int left, top, right, bottom; };
typedef void (*Panel9PatchFn)(void*, const C3DRect*, float, float, float,
                              float, float, float);
typedef void (*ManagerPrintFn)(void*, void*, float, float, unsigned long,
                               const wchar_t*, ...);
typedef void (*FontPrintFn)(void*, float, float, unsigned long,
                            const wchar_t*, ...);
static PanelCollisionFn o_PanelCollision = NULL;
static PanelDrawFn o_PanelDraw = NULL;
static Panel9PatchFn o_Panel9Patch = NULL;
static Panel9PatchFn o_PanelNew9Patch = NULL;
static ManagerPrintFn o_PrintLeft = NULL;
static ManagerPrintFn o_PrintCenter = NULL;
static ManagerPrintFn o_PrintRight = NULL;
static FontPrintFn o_FontPrintLeft = NULL;
static FontPrintFn o_FontPrintRight = NULL;
static bool g_dateTextPending = false;
static float g_dateTextShift = 0.0f;
static float g_dateLayoutScale = 1.0f;
static const bool g_showVanillaCalendarForTesting = false;

enum CachedTextKind
{
    TEXT_NONE,
    TEXT_MANAGER_LEFT,
    TEXT_MANAGER_CENTER,
    TEXT_MANAGER_RIGHT,
    TEXT_FONT_LEFT,
    TEXT_FONT_RIGHT
};

struct CachedDateText
{
    CachedTextKind kind;
    void* manager;
    void* font;
    float x;
    float y;
    unsigned long color;
    wchar_t date[128];
};

struct CachedDatePatch
{
    bool valid;
    bool useNewPatch;
    void* panel;
    C3DRect rect;
    float a, b, c, d, e, f;
};

static CachedDateText g_cachedDate = {};
static CachedDatePatch g_cachedPatch = {};
static wchar_t g_lastRenderedClockDate[256] = {};

// The game's text API is printf-like, but some vehicle and building mods pass
// strings which the secure UCRT formatter considers invalid (for example a
// literal or incomplete '%' sequence).  _vsnwprintf_s handles that by calling
// invoke_watson and terminating the entire game.  The legacy bounded formatter
// reports the failure instead, which lets us fall back to the original text.
static void FormatGameText(wchar_t* output, size_t capacity,
                           const wchar_t* format, va_list args)
{
    if (!output || capacity == 0) return;
    output[0] = L'\0';
    if (!format) return;

    const int written = _vsnwprintf(output, capacity - 1, format, args);
    output[capacity - 1] = L'\0';
    if (written < 0)
    {
        wcsncpy_s(output, capacity, format, _TRUNCATE);
    }
}

static const uintptr_t G_PANEL_POS = 0x9BE2F0;
static const uintptr_t G_PANEL_SIZE = 0x9BE2E8;
static const uintptr_t G_SCREEN_WIDTH = 0x99528C;

static uintptr_t ReturnRva(void* address)
{
    uintptr_t value = (uintptr_t)address;
    return value >= (uintptr_t)g_base ? value - (uintptr_t)g_base : value;
}

static bool IsGameplayTopHud(void* returnAddress)
{
    uintptr_t rva = ReturnRva(returnAddress);
    return rva >= 0x319D00 && rva <= 0x31D600;
}

static int ScreenWidth(void)
{
    HWND window = GetForegroundWindow();
    if (window)
    {
        DWORD processId = 0;
        GetWindowThreadProcessId(window, &processId);
        if (processId == GetCurrentProcessId())
        {
            RECT client = {};
            if (GetClientRect(window, &client))
            {
                int clientWidth = client.right - client.left;
                if (clientWidth >= 800 && clientWidth <= 16384)
                    return clientWidth;
            }
        }
    }
    int width = *(int*)(g_base + G_SCREEN_WIDTH);
    return width >= 800 && width <= 16384 ? width : 2560;
}

static bool CurrentPanelIsOldCalendarControl(void)
{
    const float* pos = (const float*)(g_base + G_PANEL_POS);
    const float* size = (const float*)(g_base + G_PANEL_SIZE);
    const float width = (float)ScreenWidth();
    return _finite(pos[0]) && _finite(pos[1]) &&
           _finite(size[0]) && _finite(size[1]) &&
           pos[0] >= width * 0.25f && pos[0] <= width * 0.59f &&
           pos[1] >= 35.0f && pos[1] <= 170.0f &&
           size[0] > 1.0f && size[1] > 1.0f;
}

static bool RectIsOldCalendarControl(const C3DRect* rect)
{
    if (!rect) return false;
    int rectWidth = rect->right - rect->left;
    int rectHeight = rect->bottom - rect->top;
    if (rectWidth <= 0 || rectHeight <= 0 ||
        rectWidth >= 10000 || rectHeight >= 2000)
        return false;
    float x = (rect->left + rect->right) * 0.5f;
    float y = (rect->top + rect->bottom) * 0.5f;
    float width = (float)ScreenWidth();
    return x >= width * 0.25f && x <= width * 0.59f &&
           y >= 35.0f && y <= 170.0f;
}

static bool RectIsDateField(const C3DRect* rect)
{
    if (!rect) return false;
    int rectWidth = rect->right - rect->left;
    int rectHeight = rect->bottom - rect->top;
    if (rectWidth <= 0 || rectHeight <= 0) return false;
    float x = (rect->left + rect->right) * 0.5f;
    float y = (rect->top + rect->bottom) * 0.5f;
    float width = (float)ScreenWidth();
    // UI element sizes follow the vertical resolution and the user's UI
    // scale, not the monitor aspect ratio.  Comparing the field width with a
    // percentage of the full screen rejected the real date field on 32:9
    // displays (5120x1440), especially with a compact custom UI scale.
    // Keep horizontal position validation, but identify the stock field by
    // its own plausible pixel size instead of the total monitor width.
    return x >= width * 0.32f && x <= width * 0.48f &&
           y >= 0.0f && y <= 50.0f &&
           rectWidth >= 80 && rectWidth <= 640 &&
           rectHeight <= 80;
}

static const C3DRect* ExtendDateField(const C3DRect* rect,
                                     C3DRect* extended,
                                     bool topHud)
{
    if (!topHud || !RectIsDateField(rect)) return rect;
    *extended = *rect;
    int originalWidth = rect->right - rect->left;
    // The stock date field is 192 px wide at the reference UI scale.
    // Its actual width follows both resolution and the user's UI scale, so it
    // is a safe anchor for every horizontal offset inside the extended field.
    float layoutScale = (float)originalWidth / 192.0f;
    if (_finite(layoutScale))
    {
        if (layoutScale < 0.45f) layoutScale = 0.45f;
        if (layoutScale > 2.50f) layoutScale = 2.50f;
        g_dateLayoutScale = layoutScale;
    }
    extended->left -= originalWidth / 12;
    extended->right += originalWidth / 6;
    return extended;
}

static bool LooksLikeDate(const wchar_t* text)
{
    if (!text) return false;
    size_t length = wcslen(text);
    if (length < 8 || length > 80) return false;
    int digits = 0;
    for (const wchar_t* p = text; *p; ++p)
    {
        if (*p >= L'0' && *p <= L'9')
        {
            if (++digits == 4) return true;
        }
        else digits = 0;
    }
    return false;
}

static bool LooksLikeMoney(const wchar_t* text)
{
    if (!text || !wcschr(text, L',')) return false;
    int digits = 0;
    for (const wchar_t* p = text; *p; ++p)
    {
        if (*p >= L'0' && *p <= L'9') ++digits;
        else if (*p != L',' && *p != L' ' && *p != 0xA0) return false;
    }
    return digits >= 4;
}

static float CenterTopValueY(float y, const wchar_t* text,
                             void* returnAddress)
{
    if (!IsGameplayTopHud(returnAddress) || !LooksLikeMoney(text)) return y;
#ifdef GAMECLOCK_24H
    // At compact UI scales the font baseline sits too close to the lower
    // border. Keep the approved 1920x1080 position and progressively lift the
    // value line, reaching a two-pixel correction at 1366x768.
    float compactLift = (1.0f - g_dateLayoutScale) * 7.0f;
    if (compactLift < 0.0f) compactLift = 0.0f;
    if (compactLift > 2.0f) compactLift = 2.0f;
    return y + 3.0f - compactLift;
#else
    float compactLift = (1.0f - g_dateLayoutScale) * 7.0f;
    if (compactLift < 0.0f) compactLift = 0.0f;
    if (compactLift > 2.0f) compactLift = 2.0f;
    return y + 3.0f - compactLift;
#endif
}

static float DateTextY(float y)
{
#ifdef GAMECLOCK_24H
    float compactLift = (1.0f - g_dateLayoutScale) * 7.0f;
    if (compactLift < 0.0f) compactLift = 0.0f;
    if (compactLift > 2.0f) compactLift = 2.0f;
    return y + 3.0f - compactLift;
#else
    float compactLift = (1.0f - g_dateLayoutScale) * 7.0f;
    if (compactLift < 0.0f) compactLift = 0.0f;
    if (compactLift > 2.0f) compactLift = 2.0f;
    return y + 3.0f - compactLift;
#endif
}

static void FormatClock(wchar_t* output, size_t capacity)
{
    float progress = -1.0f;
    // Calendar Synchronizer 2.1 no longer exposes the private cache used by
    // version 1.0. Its supported source of truth is the game's own 0..60
    // time-in-day field, which the synchronizer deliberately keeps current.
    unsigned char* world = g_base + G_WORLD_OBJECT;
    progress = *(float*)(world + 0x59C);
    if (!_finite(progress) || progress < 0.0f || progress > 60.5f)
        progress = 0.0f;
    // daynight offset=0.5825 rotates the measured lighting cycle so the
    // calendar's 60->0 rollover is midnight. New transition positions:
    // dawn t=6.60 -> 05:00, full day t=11.20 -> 07:00,
    // sunset t=52.75 -> 20:00, night t=57.36 -> 22:00.
    float hour = 0.0f;
    if (progress < 6.60f)
        hour = progress * (5.0f / 6.60f);
    else if (progress < 11.20f)
        hour = 5.0f + (progress - 6.60f) * (2.0f / 4.60f);
    else if (progress < 52.75f)
        hour = 7.0f + (progress - 11.20f) * (13.0f / 41.55f);
    else if (progress < 57.36f)
        hour = 20.0f + (progress - 52.75f) * (2.0f / 4.61f);
    else
        hour = 22.0f + (progress - 57.36f) * (2.0f / 2.64f);
    hour = fmodf(hour, 24.0f);
    if (hour < 0.0f) hour += 24.0f;
    int halfHour = (int)floorf(hour * 2.0f + 0.0001f) % 48;
    int hour24 = halfHour / 2;
#ifdef GAMECLOCK_24H
    _snwprintf_s(output, capacity, _TRUNCATE, L"%02d:%02d",
                 hour24, (halfHour & 1) ? 30 : 0);
#else
    int hour12 = hour24 % 12;
    if (!hour12) hour12 = 12;
    _snwprintf_s(output, capacity, _TRUNCATE, L"%d:%02d %ls",
                 hour12, (halfHour & 1) ? 30 : 0,
                 hour24 < 12 ? L"AM" : L"PM");
#endif
}

static bool ConsumeDateText(float x, const wchar_t* text, float* shiftedX)
{
    *shiftedX = x;
    if (!g_dateTextPending || !LooksLikeDate(text)) return false;
    g_dateTextPending = false;
    *shiftedX = x + g_dateTextShift;
    return true;
}

static void BuildClockDate(wchar_t* output, size_t capacity,
                           const wchar_t* date)
{
    wchar_t clock[16] = {};
    FormatClock(clock, 16);
#ifdef GAMECLOCK_24H
    _snwprintf_s(output, capacity, _TRUNCATE, L"%ls   |  %ls", clock, date);
#else
    // AM/PM is wider, so two spaces keep its left and right margins balanced.
    _snwprintf_s(output, capacity, _TRUNCATE, L"%ls  |  %ls", clock, date);
#endif
}

static void PrintSeparatedClockDate(CachedTextKind kind, void* manager,
                                    void* font, float x, float y,
                                    unsigned long color,
                                    const wchar_t* date)
{
    wchar_t clock[16] = {};
    FormatClock(clock, 16);
#ifdef GAMECLOCK_24H
    // Preserve the approved 24-hour positions. Only the separator is drawn
    // separately so it can share the AM/PM variant's vertical level.
    if (kind == TEXT_MANAGER_RIGHT && manager)
    {
        const float scale = g_dateLayoutScale;
        o_PrintRight(manager, font, x, y, color, L"%ls", date);
        o_PrintRight(manager, font, x - 168.0f * scale,
                     y - 2.0f * scale, color, L"|");
        o_PrintRight(manager, font, x - 183.0f * scale,
                     y, color, L"%ls", clock);
        return;
    }
    wchar_t combined[256] = {};
    BuildClockDate(combined, 256, date);
    if (kind == TEXT_MANAGER_LEFT)
        o_PrintLeft(manager, font, x, y, color, L"%ls", combined);
    else if (kind == TEXT_MANAGER_CENTER)
        o_PrintCenter(manager, font, x, y, color, L"%ls", combined);
    else if (kind == TEXT_MANAGER_RIGHT)
        o_PrintRight(manager, font, x, y, color, L"%ls", combined);
    else if (kind == TEXT_FONT_LEFT)
        o_FontPrintLeft(font, x, y, color, L"%ls", combined);
    else
        o_FontPrintRight(font, x, y, color, L"%ls", combined);
    return;
#else
    if (kind == TEXT_MANAGER_RIGHT && manager)
    {
        // The visible HUD label is right-aligned (renderer kind 3). Keep that
        // exact coordinate system for every part; center-aligned printing is
        // invalid here because the HUD uses a translated negative X origin.
        const float scale = g_dateLayoutScale;
        const float rightAnchor = x + 4.0f * scale;
        o_PrintRight(manager, font, rightAnchor, y, color, L"%ls", date);
        o_PrintRight(manager, font, rightAnchor - 158.0f * scale,
                     y - 2.0f * scale,
                     color, L"|");
        o_PrintRight(manager, font, rightAnchor - 163.0f * scale, y,
                     color, L"%ls", clock);
        return;
    }

    {
        wchar_t combined[256] = {};
        BuildClockDate(combined, 256, date);
        if (kind == TEXT_MANAGER_LEFT)
            o_PrintLeft(manager, font, x, y, color, L"%ls", combined);
        else if (kind == TEXT_MANAGER_CENTER)
            o_PrintCenter(manager, font, x, y, color, L"%ls", combined);
        else if (kind == TEXT_MANAGER_RIGHT)
            o_PrintRight(manager, font, x, y, color, L"%ls", combined);
        else if (kind == TEXT_FONT_LEFT)
            o_FontPrintLeft(font, x, y, color, L"%ls", combined);
        else
            o_FontPrintRight(font, x, y, color, L"%ls", combined);
    }
#endif
}

static void CacheManagerDate(CachedTextKind kind, void* manager, void* font,
                             float x, float y, unsigned long color,
                             const wchar_t* date)
{
    g_cachedDate.kind = kind;
    g_cachedDate.manager = manager;
    g_cachedDate.font = font;
    g_cachedDate.x = x;
    g_cachedDate.y = y;
    g_cachedDate.color = color;
    wcsncpy_s(g_cachedDate.date, 128, date, _TRUNCATE);
    BuildClockDate(g_lastRenderedClockDate, 256, date);
}

static void CacheFontDate(CachedTextKind kind, void* font, float x, float y,
                          unsigned long color, const wchar_t* date)
{
    CacheManagerDate(kind, NULL, font, x, y, color, date);
}

static void RenderCachedClockDate(void)
{
    if (!g_cachedPatch.valid || g_cachedDate.kind == TEXT_NONE) return;

    wchar_t combined[256] = {};
    BuildClockDate(combined, 256, g_cachedDate.date);
    if (wcscmp(combined, g_lastRenderedClockDate) == 0) return;

    if (g_cachedPatch.useNewPatch)
        o_PanelNew9Patch(g_cachedPatch.panel, &g_cachedPatch.rect,
                         g_cachedPatch.a, g_cachedPatch.b, g_cachedPatch.c,
                         g_cachedPatch.d, g_cachedPatch.e, g_cachedPatch.f);
    else
        o_Panel9Patch(g_cachedPatch.panel, &g_cachedPatch.rect,
                      g_cachedPatch.a, g_cachedPatch.b, g_cachedPatch.c,
                      g_cachedPatch.d, g_cachedPatch.e, g_cachedPatch.f);

    PrintSeparatedClockDate(g_cachedDate.kind, g_cachedDate.manager,
                            g_cachedDate.font,
                            g_cachedDate.x, g_cachedDate.y,
                            g_cachedDate.color, g_cachedDate.date);
    wcsncpy_s(g_lastRenderedClockDate, 256, combined, _TRUNCATE);
}

static void h_PrintLeft(void* manager, void* font, float x, float y,
                        unsigned long color, const wchar_t* format, ...)
{
    wchar_t text[2048] = {};
    va_list args;
    va_start(args, format);
    FormatGameText(text, 2048, format, args);
    va_end(args);
    float shiftedX = x;
    float drawY = CenterTopValueY(y, text, __builtin_return_address(0));
    if (ConsumeDateText(x, text, &shiftedX))
    {
        drawY = DateTextY(y);
        CacheManagerDate(TEXT_MANAGER_LEFT, manager, font, shiftedX, drawY,
                         color, text);
        PrintSeparatedClockDate(TEXT_MANAGER_LEFT, manager, font,
                                shiftedX, drawY,
                                color, text);
        return;
    }
    o_PrintLeft(manager, font, shiftedX, drawY, color, L"%ls", text);
}

static void h_PrintCenter(void* manager, void* font, float x, float y,
                          unsigned long color, const wchar_t* format, ...)
{
    wchar_t text[2048] = {};
    va_list args;
    va_start(args, format);
    FormatGameText(text, 2048, format, args);
    va_end(args);
    float shiftedX = x;
    float drawY = CenterTopValueY(y, text, __builtin_return_address(0));
    if (ConsumeDateText(x, text, &shiftedX))
    {
        drawY = DateTextY(y);
        CacheManagerDate(TEXT_MANAGER_CENTER, manager, font, shiftedX, drawY,
                         color, text);
        PrintSeparatedClockDate(TEXT_MANAGER_CENTER, manager, font,
                                shiftedX, drawY,
                                color, text);
        return;
    }
    o_PrintCenter(manager, font, shiftedX, drawY, color, L"%ls", text);
}

static void h_PrintRight(void* manager, void* font, float x, float y,
                         unsigned long color, const wchar_t* format, ...)
{
    wchar_t text[2048] = {};
    va_list args;
    va_start(args, format);
    FormatGameText(text, 2048, format, args);
    va_end(args);
    float shiftedX = x;
    float drawY = CenterTopValueY(y, text, __builtin_return_address(0));
    if (ConsumeDateText(x, text, &shiftedX))
    {
        drawY = DateTextY(y);
        CacheManagerDate(TEXT_MANAGER_RIGHT, manager, font, shiftedX, drawY,
                         color, text);
        PrintSeparatedClockDate(TEXT_MANAGER_RIGHT, manager, font,
                                shiftedX, drawY,
                                color, text);
        return;
    }
    o_PrintRight(manager, font, shiftedX, drawY, color, L"%ls", text);
}

static void h_FontPrintLeft(void* font, float x, float y, unsigned long color,
                            const wchar_t* format, ...)
{
    wchar_t text[2048] = {};
    va_list args;
    va_start(args, format);
    FormatGameText(text, 2048, format, args);
    va_end(args);
    float shiftedX = x;
    float drawY = CenterTopValueY(y, text, __builtin_return_address(0));
    if (ConsumeDateText(x, text, &shiftedX))
    {
        drawY = DateTextY(y);
        CacheFontDate(TEXT_FONT_LEFT, font, shiftedX, drawY, color, text);
        PrintSeparatedClockDate(TEXT_FONT_LEFT, NULL, font, shiftedX, drawY,
                                color, text);
        return;
    }
    o_FontPrintLeft(font, shiftedX, drawY, color, L"%ls", text);
}

static void h_FontPrintRight(void* font, float x, float y,
                             unsigned long color, const wchar_t* format, ...)
{
    wchar_t text[2048] = {};
    va_list args;
    va_start(args, format);
    FormatGameText(text, 2048, format, args);
    va_end(args);
    float shiftedX = x;
    float drawY = CenterTopValueY(y, text, __builtin_return_address(0));
    if (ConsumeDateText(x, text, &shiftedX))
    {
        drawY = DateTextY(y);
        CacheFontDate(TEXT_FONT_RIGHT, font, shiftedX, drawY, color, text);
        PrintSeparatedClockDate(TEXT_FONT_RIGHT, NULL, font, shiftedX, drawY,
                                color, text);
        return;
    }
    o_FontPrintRight(font, shiftedX, drawY, color, L"%ls", text);
}

static void h_PanelDraw(void* panel, float u0, float v0, float u1, float v1,
                        float rotation, bool alpha)
{
    uintptr_t rva = ReturnRva(__builtin_return_address(0));
    if (!g_showVanillaCalendarForTesting &&
        (rva == 0x31C1FA || rva == 0x31C3A6 ||
         (IsGameplayTopHud(__builtin_return_address(0)) &&
          CurrentPanelIsOldCalendarControl())))
    {
        RenderCachedClockDate();
        return;
    }
    o_PanelDraw(panel, u0, v0, u1, v1, rotation, alpha);
    if (rva == 0x31C3A6) RenderCachedClockDate();
}

static void h_Panel9Patch(void* panel, const C3DRect* rect, float a, float b,
                          float c, float d, float e, float f)
{
    void* caller = __builtin_return_address(0);
    bool topHud = IsGameplayTopHud(caller);
    bool dateFieldCall = ReturnRva(caller) == 0x3357BB;
    if (!g_showVanillaCalendarForTesting && topHud &&
        RectIsOldCalendarControl(rect))
        return;
    C3DRect extended = {};
    if (dateFieldCall && RectIsDateField(rect))
    {
        float originalWidth = (float)(rect->right - rect->left);
        g_dateTextShift = originalWidth / 6.0f;
        g_dateTextPending = true;
        g_cachedPatch.valid = true;
        g_cachedPatch.useNewPatch = false;
        g_cachedPatch.panel = panel;
        g_cachedPatch.rect = *ExtendDateField(rect, &extended, true);
        g_cachedPatch.a = a; g_cachedPatch.b = b; g_cachedPatch.c = c;
        g_cachedPatch.d = d; g_cachedPatch.e = e; g_cachedPatch.f = f;
    }
    o_Panel9Patch(panel, ExtendDateField(rect, &extended,
                                         topHud || dateFieldCall),
                  a, b, c, d, e, f);
}

static void h_PanelNew9Patch(void* panel, const C3DRect* rect, float a,
                             float b, float c, float d, float e, float f)
{
    void* caller = __builtin_return_address(0);
    bool topHud = IsGameplayTopHud(caller);
    bool dateFieldCall = ReturnRva(caller) == 0x3357BB;
    if (!g_showVanillaCalendarForTesting && topHud &&
        RectIsOldCalendarControl(rect))
        return;
    C3DRect extended = {};
    if (dateFieldCall && RectIsDateField(rect))
    {
        float originalWidth = (float)(rect->right - rect->left);
        g_dateTextShift = originalWidth / 6.0f;
        g_dateTextPending = true;
        g_cachedPatch.valid = true;
        g_cachedPatch.useNewPatch = true;
        g_cachedPatch.panel = panel;
        g_cachedPatch.rect = *ExtendDateField(rect, &extended, true);
        g_cachedPatch.a = a; g_cachedPatch.b = b; g_cachedPatch.c = c;
        g_cachedPatch.d = d; g_cachedPatch.e = e; g_cachedPatch.f = f;
    }
    o_PanelNew9Patch(panel, ExtendDateField(rect, &extended,
                                            topHud || dateFieldCall),
                     a, b, c, d, e, f);
}

static bool h_PanelCollision(void* panel, void* mouse, float x, float y)
{
    void* caller = __builtin_return_address(0);
    uintptr_t rva = ReturnRva(caller);

    // Only the calendar progress strip uses this collision call. Keeping the
    // filter tied to its exact call site prevents speed-button clicks from
    // falling through to the world and deselecting an open building window.
    if (!g_showVanillaCalendarForTesting && rva == 0x31C246)
        return false;

    return o_PanelCollision(panel, mouse, x, y);
}

extern "C" __declspec(dllexport) unsigned TsmPluginApiVersion(void)
{
    return TSM_API_VERSION;
}

extern "C" __declspec(dllexport) int TsmPluginInit(const TsmHost* host,
                                                   TsmPluginInfo* info)
{
    H = host;
    g_base = host->exeBase;
    info->name = "Tesmio Clock";
#ifdef GAMECLOCK_24H
    info->version = "1.1.3 (24-hour)";
    H->log("Tesmio Clock 1.1.3: 24-hour variant");
#else
    info->version = "1.1.3 (AM/PM)";
    H->log("Tesmio Clock 1.1.3: AM/PM variant");
#endif
    return 0;
}

extern "C" __declspec(dllexport) int TsmPluginStart(void)
{
    HMODULE engineModule = GetModuleHandleA(ENGINE_DLL);
    if (!engineModule)
    {
        H->log("Tesmio Clock: engine module not loaded");
        return 1;
    }
    void** collisionSlot = H->findIatSlot(H->exeModule, ENGINE_DLL,
                                          PANEL_COLLISION);
    void** drawSlot = H->findIatSlot(H->exeModule, ENGINE_DLL, PANEL_DRAW);
    void** patchSlot = H->findIatSlot(H->exeModule, ENGINE_DLL, PANEL_9PATCH);
    void** newPatchSlot = H->findIatSlot(H->exeModule, ENGINE_DLL,
                                         PANEL_NEW_9PATCH);
    void** printLeftSlot = H->findIatSlot(H->exeModule, ENGINE_DLL, PRINT_LEFT);
    void** printCenterSlot = H->findIatSlot(H->exeModule, ENGINE_DLL,
                                            PRINT_CENTER);
    void** printRightSlot = H->findIatSlot(H->exeModule, ENGINE_DLL,
                                           PRINT_RIGHT);
    void** fontLeftSlot = H->findIatSlot(H->exeModule, ENGINE_DLL,
                                         FONT_PRINT_LEFT);
    void** fontRightSlot = H->findIatSlot(H->exeModule, ENGINE_DLL,
                                          FONT_PRINT_RIGHT);
    if (!collisionSlot || !*collisionSlot || !drawSlot || !*drawSlot ||
        !patchSlot || !*patchSlot || !newPatchSlot || !*newPatchSlot ||
        !printLeftSlot || !*printLeftSlot ||
        !printCenterSlot || !*printCenterSlot ||
        !printRightSlot || !*printRightSlot ||
        !fontLeftSlot || !*fontLeftSlot || !fontRightSlot || !*fontRightSlot)
    {
        H->log("Tesmio Clock: missing engine import");
        return 1;
    }
    o_PanelCollision = (PanelCollisionFn)*collisionSlot;
    o_PanelDraw = (PanelDrawFn)*drawSlot;
    o_Panel9Patch = (Panel9PatchFn)*patchSlot;
    o_PanelNew9Patch = (Panel9PatchFn)*newPatchSlot;
    o_PrintLeft = (ManagerPrintFn)*printLeftSlot;
    o_PrintCenter = (ManagerPrintFn)*printCenterSlot;
    o_PrintRight = (ManagerPrintFn)*printRightSlot;
    o_FontPrintLeft = (FontPrintFn)*fontLeftSlot;
    o_FontPrintRight = (FontPrintFn)*fontRightSlot;

    bool ok = H->patchIat(H->exeModule, ENGINE_DLL, PANEL_COLLISION,
                          (void*)h_PanelCollision,
                          (void**)&o_PanelCollision,
                          "calendar passive hover") != 0;
    ok &= H->patchIat(H->exeModule, ENGINE_DLL, PANEL_DRAW,
                      (void*)h_PanelDraw, (void**)&o_PanelDraw,
                      "calendar strip sprites") != 0;
    ok &= H->patchIat(H->exeModule, ENGINE_DLL, PANEL_9PATCH,
                      (void*)h_Panel9Patch, (void**)&o_Panel9Patch,
                      "calendar strip background") != 0;
    ok &= H->patchIat(H->exeModule, ENGINE_DLL, PANEL_NEW_9PATCH,
                      (void*)h_PanelNew9Patch, (void**)&o_PanelNew9Patch,
                      "calendar strip new background") != 0;
    ok &= H->patchIat(H->exeModule, ENGINE_DLL, PRINT_LEFT,
                      (void*)h_PrintLeft, (void**)&o_PrintLeft,
                      "date text left") != 0;
    ok &= H->patchIat(H->exeModule, ENGINE_DLL, PRINT_CENTER,
                      (void*)h_PrintCenter, (void**)&o_PrintCenter,
                      "date text center") != 0;
    ok &= H->patchIat(H->exeModule, ENGINE_DLL, PRINT_RIGHT,
                      (void*)h_PrintRight, (void**)&o_PrintRight,
                      "date text right") != 0;
    ok &= H->patchIat(H->exeModule, ENGINE_DLL, FONT_PRINT_LEFT,
                      (void*)h_FontPrintLeft, (void**)&o_FontPrintLeft,
                      "date direct left") != 0;
    ok &= H->patchIat(H->exeModule, ENGINE_DLL, FONT_PRINT_RIGHT,
                      (void*)h_FontPrintRight, (void**)&o_FontPrintRight,
                      "date direct right") != 0;
    H->log("Tesmio Clock: %s", ok ? "ready" : "install failed");
    return ok ? 0 : 1;
}
