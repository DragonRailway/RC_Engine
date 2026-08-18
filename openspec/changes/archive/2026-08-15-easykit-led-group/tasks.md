# Tasks

## Section 1: EasyLEDGroup in the EasyKit library

- [x] 1.1 Implement `EasyLEDGroup` in `lib/ESP32_EasyKit` (new `EasyLEDGroup.h/.cpp`): step-table pattern (per-member duty 0–100% + duration), shared `millis()` timeline, `startPattern()`, `stop()` (members to 0, idle), `isRunning()`, `update()`; construct with an initializer list of `EasyLED*`
- [x] 1.2 Add built-in pattern factories: `alternate(intervalMs)`, `syncFlash(onMs, offMs)`, `chase(intervalMs)`, `doubleStrobe(intervalMs, gapMs)`
- [x] 1.3 Document the class + patterns in `lib/ESP32_EasyKit/README.md`


## Section 2: Ditch-light migration + pump completeness

- [x] 2.1 Bind a two-member `EasyLEDGroup` to `ditchLLed`/`ditchRLed` in `HardwareInit`; start `alternate(intervalMs)` from `initLights()` when configured
- [x] 2.2 Replace the manual alternation state in `VehicleController` (`s_ditchLastToggle`, `s_ditchSide`) with group start/stop driven by the ditch app toggle (bit 5); edge-triggered, no-op on unchanged state
- [x] 2.3 Add `ditchLLed`, `ditchRLed`, `stepLed`, `cabLed`, and the ditch group to `HardwareInit::update()` pump; confirm hot-reload teardown and battery-cutoff paths call `group.stop()`


## Section 3: Docs + validation

- [x] 3.1 Update `GUIDE/HARDWARE_CONFIG.md` §4.5: ditch light now runs the group's `alternate` pattern (config keys unchanged); note the pattern factories for future vehicles
- [x] 3.2 Confirm parser/schema untouched (ditch config keys unchanged) and `validate_configs.py` still passes for all configs


## Section 4: Verification

- [x] 4.1 Build both environments (MIKRO_V2, TRACKLINK_V3) and run host VC tests

- [x] 4.2 Bench/hardware verification: ditch pair alternates at `interval_ms` while toggled on, both off when toggled off; turn signals and headlight fade unaffected

