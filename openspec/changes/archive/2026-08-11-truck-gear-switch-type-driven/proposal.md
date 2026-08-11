## Why

The RadioKit UI presents two mutually exclusive vehicle models (Truck page / Loco page), and `/vehicle-config.json` declares which model the device actually is (`type: TRUCK | LOCOMOTIVE | ...`) — never both. The firmware currently selects input widgets by the *active RadioKit page* instead of the configured vehicle type, which is the wrong axis. In addition, the Truck page's `gear_switch` (a single-select radio group) is completely unwired, and the manual indicator buttons were changed in the app to latched toggles.

## What Changes

- **Vehicle-type-driven widget selection**: all input reads gate on `profile.config.type` (TRUCK widgets vs LOCOMOTIVE widgets), replacing the active-page branching. The two widget sets are exclusive and never active simultaneously.
- **Truck gear switch D/P/R** (radio indices D=0, P=1, R=2):
  - **D (Drive)**: motor follows the gas pedal forward.
  - **P (Park)**: motor locked to 0 regardless of throttle, with the parking-brake sound (`triggerParkingBrake` — currently unwired).
  - **R (Reverse)**: motor reverses, the reversing beep plays (`triggerReversing`), and the reversing light is lit automatically (Item E stays as a manual override).
  - **Shifting sound**: `triggerShifting` on gear change, only while the engine is RUNNING.
- **Manual indicator toggles**: `left_indicator` / `right_indicator` become latched toggle buttons (re-imported from the app); the existing merge with steering auto turn-signals stands unchanged.
- **Loco unchanged**: `dir_switch` remains the sole direction authority for locomotives.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `vehicle-control-loop`: input widget selection driven by vehicle type instead of active page; truck gear-switch direction/park/shift behavior in "Direction and braking".
- `advanced-lighting-automation`: manual turn-signal toggle buttons (latched) merged with steering auto signals; truck gear R automatically illuminates the reversing light.

## Impact

- `common/VehicleController.h` — type-based widget gating, gear state machine (D/P/R), park lock + parking-brake sound, reverse handling + reversing-light automation, shift sound.
- `src/RADIOKIT.h` + `docs/radiokit-rc-ui-design.json` — re-imported from the app (indicators → toggles, gear labels D/P/R), patched via `scripts/patch_radiokit_header.py`.
- No config schema changes — `RcEngineSound::Config.type` is already parsed from `vehicle-config.json`.
