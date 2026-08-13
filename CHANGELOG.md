# Changelog

## 1.1.4 — 2026-08-14

- Fixed vehicles moving many times faster than intended after installing
  Tesmio Clock alongside Calendar Synchronizer.
- The installer now explicitly restores `vehicle_scale = 1`, keeping the base
  game's vehicle speed while Calendar Synchronizer stretches the calendar day.
- The fix is applied whenever the installer is run and works with both the
  24-hour and AM/PM variants.

## 1.1.3 — 2026-08-12

- Added adaptive placement for super-ultrawide and multi-monitor resolutions.
- Removed the date-field width check that incorrectly depended on the total
  screen width and could place the clock over the currency field at 32:9.
- Verified the top-bar layout in game at 3840×1080 and 5120×1440.
- Added support for 5760×1080 and 7680×2160 layouts through the same adaptive
  date-field detection.
- The update applies to both the 24-hour and AM/PM variants.

## 1.1.2 — 2026-08-12

- Fixed a crash when selecting aircraft, horse-drawn vehicles from other mods,
  and horse-drawn vehicles included with the Early Start DLC.
- Made all clock text hooks tolerate non-standard game and mod text instead of
  terminating the game when such a vehicle window is opened.
- The fix applies to both the 24-hour and AM/PM variants.

## 1.1.1 — 2026-08-11

- Updated for Workers & Resources: Soviet Republic 1.1.1.9,
  TesmioLoader b0.3.6 / plugin API 4, and Calendar Synchronizer 2.1.
- Restored the live clock after Calendar Synchronizer changed its internal
  timer storage; both 24-hour and AM/PM variants now use the verified game
  world timer maintained by the synchronizer.
- Restored reliable clock placement after the 10 August 2026 game update.
- The top-bar field now uses the active game window width as its primary
  layout anchor instead of relying only on a fixed internal screen address.
- Added support for compact windows and the game's custom interface scale.

## 1.1.0 — 2026-08-10

- Added adaptive clock and date placement based on the actual top-bar field size.
- Tested 24-hour and AM/PM layouts at 1366×768, 1440×900, 1600×900,
  1920×1080 and 2560×1440.
- Added support for the game's custom interface scale setting.
- Fully removed the remaining visual layer of the vanilla calendar strip.
- Fixed building windows closing when the game speed was changed from the top bar.

## 1.0.0 — 2026-08-10

- Initial public release.
- Added a synchronized half-hour digital clock to the top HUD.
- Added separately tested 24-hour and AM/PM variants.
- Added automatic format selection and installation.
- Added Calendar Synchronizer detection, backup and tested calibration.
- Disabled the vanilla calendar hover strip.
- Centered top-bar currency and date values vertically.
