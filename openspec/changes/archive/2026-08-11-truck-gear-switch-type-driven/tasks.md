## 1. UI Re-import

- [x] 1.1 Re-import `docs/radiokit-rc-ui-design.json` + `src/RADIOKIT.h` from the RadioKit API (design `1785927365527`) and run `scripts/patch_radiokit_header.py`
- [x] 1.2 Verify `left_indicator`/`right_indicator` are `RK_ToggleButton` and `gear_switch` is a 3-item radio with D/P/R labels/order — **app-side pending** (API still serves push + A/B/C; firmware is forward-compatible)

## 2. Type-Driven Widget Selection

- [x] 2.1 Add `isLoco` gate from `profile.config.type` and branch throttle, lights bitmask, engine toggle, horn/bell, and aux_slider reads on it in `VehicleController::update()`
- [x] 2.2 Remove the `RadioKit.getActivePage()`-based widget selection (page no longer drives input mapping)

## 3. Truck Gear Switch D/P/R

- [x] 3.1 Decode `gear_switch.rk.value` radio index (D=0, P=1, R=2, out-of-range → P); add `s_gearPrev` + `s_parkingBrakePrev` state
- [x] 3.2 Park: force `motorThrottle = 0` before the brake blend and call `triggerParkingBrake` with change detection
- [x] 3.3 Reverse: set `reverse` from gear R (truck), negating motor, `triggerReversing`, and auto-lit reversing lamp (`manualRev = (bits & 0x10) || reverse`)
- [x] 3.4 Shift sound: `triggerShifting` on gear-change edge only while engine RUNNING

## 4. Validation

- [x] 4.1 Build `pio run -e TRACKLINK_V3` and `pio run -e MIKRO_V2` with no errors
- [x] 4.2 Review the diff against the delta specs (each scenario has matching behavior in code) — reviewer fixes applied: skid-steer Park lock, isLoco-gated truck widget reads, isLoco-gated indicator buttons
