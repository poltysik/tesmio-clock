# Tesmio Clock 1.1.2

Tesmio Clock adds a readable digital clock to the top interface of
**Workers & Resources: Soviet Republic**. It is an add-on for
[Calendar Synchronizer](https://steamcommunity.com/sharedfiles/filedetails/?id=3779646468)
and uses the synchronizer's real in-game day/night position rather than an
independent timer.

> **Important: this is not a standalone or full time-synchronization mod.**
> Tesmio Clock is only an interface add-on for
> [Calendar Synchronizer by Tesmio / MaxLegend](https://steamcommunity.com/sharedfiles/filedetails/?id=3779646468).
> Subscribe to and install the original author's mod first. All calendar-speed
> and day/night synchronization logic belongs to Calendar Synchronizer; this
> project adds the clock display and its calibrated integration.

## What's new in 1.1.2

- Fixed a crash when selecting aircraft, horse-drawn vehicles from other mods,
  and horse-drawn vehicles from the Early Start DLC.
- Clock text processing now safely handles non-standard game and mod text.
- The fix is included in both the 24-hour and AM/PM variants.

## Previous 1.1.1 compatibility update

- Updated for Workers & Resources: Soviet Republic `1.1.1.9`, TesmioLoader
  `b0.3.6` / plugin API 4, and Calendar Synchronizer `2.1`.
- Restored live time updates in both 24-hour and AM/PM variants after the
  Calendar Synchronizer update.
- In-game verification at `1366×768`, `1440×900`, `1600×900`,
  `1920×1080` and `2560×1440`.
- Support for the game's custom interface scale setting.
- Correct alignment for both 24-hour and AM/PM formats at compact UI scales.
- Complete removal of the remaining vanilla calendar strip graphics.
- Fixed building windows closing when changing the game speed from the top bar.

The clock advances in 30-minute steps and is calibrated to the visual cycle:
dawn begins around 05:00, daylight around 07:00, sunset around 20:00 and night
around 22:00. The calendar date changes at 00:00.

## Features

- Two separately tested display formats:
  - `16:00` — 24-hour format;
  - `4:00 PM` — 12-hour AM/PM format.
- Half-hour updates with no rapidly changing minute counter.
- Date and time in one extended stock-style field.
- The original calendar hover strip is disabled.
- Top-bar currency and date values are vertically centered.
- One day/night cycle per calendar day when used with Calendar Synchronizer.
- No save-game changes and no modification of `SOVIET64.exe` on disk.

## Requirements

- Workers & Resources: Soviet Republic `1.1.1.9`, 64-bit DX11 build.
- [TesmioLoader](https://steamcommunity.com/sharedfiles/filedetails/?id=3773169177)
  API 4 / launcher `b0.3.6`.
- [Calendar Synchronizer](https://steamcommunity.com/sharedfiles/filedetails/?id=3779646468)
  version `2.1`, installed as `daynight.dll` and `daynight.ini` in the
  TesmioLoader plugin folder.
- Start the game through `tesmiolauncher.exe`.

The plugin uses engine addresses from the supported game build. A game or
TesmioLoader update may require a new release.

## Automatic installation

1. Install TesmioLoader and Calendar Synchronizer first.
2. Download and extract `TesmioClock-1.1.2.zip`, or open the Workshop item folder.
3. Run `INSTALL-TESMIO-CLOCK.bat`.
4. Choose:
   - `1` for 24-hour time;
   - `2` for AM/PM.
5. Start `tesmiolauncher.exe`, enable `TesmioClock.dll`, and press **Launch**.

The installer locates the game, installs exactly one clock DLL, disables the
legacy development file `GameClock.dll`, verifies the copied DLL, and applies
the tested Calendar Synchronizer calibration. Before changing `daynight.ini`,
it creates `daynight.ini.tesmioclock-backup` once.

To change the format later, run the installer again. The default choice is
stored in `clock-format.ini`; it can also be edited manually to either:

```ini
[clock]
format = 24-hour
```

or:

```ini
[clock]
format = 12-hour-am-pm
```

## Manual installation

Copy the selected file:

```text
variants\24-hour\TesmioClock.dll
```

or:

```text
variants\12-hour-am-pm\TesmioClock.dll
```

to:

```text
SovietRepublic\tesmioloader\build\plugins\TesmioClock.dll
```

Only one variant may be present. Remove or disable the old `GameClock.dll`.
For the tested synchronization, Calendar Synchronizer must use:

```ini
enabled = 1
cycle_days = 1
day_scale = auto
offset = 0.5825
fade = 1
probe = 0
```

## Troubleshooting

- Confirm the game is launched through `tesmiolauncher.exe`.
- Confirm `TesmioClock.dll` and `daynight.dll` are enabled.
- Do not enable both `GameClock.dll` and `TesmioClock.dll`.
- Check `tesmioloader.log` for `Tesmio Clock`, `missing engine import`, or
  `install failed`.
- Fully restart the game and launcher after changing formats.

## Building from source

Place an `llvm-mingw` toolchain in `tools\llvm-mingw`, then run:

```powershell
.\build-portable.ps1
.\package-release.ps1 -Version 1.1.2
```

## Russian documentation

See [GUIDE-RU.md](GUIDE-RU.md).

## Credits

Calendar Synchronizer and TesmioLoader are created by **Tesmio / MaxLegend**.
Thank you to the original author for the synchronization plugin that makes this
clock add-on possible.

## License

GPL-3.0-or-later. TesmioLoader ABI declarations are derived from the GPL-3.0
TesmioLoader project by MaxLegend.
