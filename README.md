# Tesmio Clock 1.1.4

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

## What's new in 1.1.4

- Fixed vehicles moving many times faster than intended after installation.
- The installer now enforces `vehicle_scale = 1` in Calendar Synchronizer,
  preserving the base game's vehicle speed while the calendar day remains
  synchronized and extended.
- The correction is applied to both the 24-hour and AM/PM variants.

## Previous 1.1.3 ultrawide update

- Added adaptive placement for super-ultrawide and multi-monitor layouts.
- Fixed clock placement at 32:9: the date field is now identified by its actual
  UI geometry instead of a percentage of the full monitor width.
- Verified in game at `3840×1080` and `5120×1440`.
- Added support for `5760×1080` and `7680×2160` through the same adaptive
  placement logic.
- The update is included in both the 24-hour and AM/PM variants.

## Previous 1.1.2 vehicle compatibility fix

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

## Checking the installed version

- `TESMIO-CLOCK-VERSION.txt` in the Workshop item folder shows the package
  version downloaded by Steam.
- `tesmioloader\build\plugins\TesmioClock.version.txt` shows the version and
  display format actually copied into TesmioLoader by the installer.
- If these versions differ, close the game and run `INSTALL-TESMIO-CLOCK.bat`
  again.

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
2. Download and extract `TesmioClock-1.1.3.zip`, or open the Workshop item folder.
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
slow = calendar
vehicle_scale = 1
sim_scale = 1
offset = 0.5825
fade = 1
probe = 0
```

## Troubleshooting

- **Vehicles move far too fast:** this is caused by Calendar Synchronizer's
  vehicle-speed setting, not by the clock display. Close the game and open:

  ```text
  Steam\steamapps\common\SovietRepublic\tesmioloader\build\plugins\daynight.ini
  ```

  Replace `vehicle_scale = 0.125` with `vehicle_scale = 1`, save the file, and
  start the game through `tesmiolauncher.exe`. The Tesmio Clock installer does
  this automatically. If Calendar Synchronizer is reinstalled or updated
  afterwards and the issue returns, run `INSTALL-TESMIO-CLOCK.bat` again.
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
.\package-release.ps1 -Version 1.1.3
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
