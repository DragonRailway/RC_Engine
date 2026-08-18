# Tasks: Board Power Management & 3-State Power Control

## Section 1: Schema & Config Parser

- [x] 1.1 Update `configs/schemas/hardware_config.schema.json`: add `"power"` block with `boot_latch_s` (default 1.0, range 0–30s), `button_hold_s` (default 4.0, range 1–30s), `disconnect_timeout_s` (default 60.0, range 0–3600s), `warning_window_s` (default 10.0, range 1–60s), and `cutoff_delay_s` (default 1.5, range 0–60s)
- [x] 1.2 Update `common/Config.h`: replace hardcoded power timings with `HardwareConfig::Power` struct using seconds units
- [x] 1.3 Update `common/ConfigParser.h`: parse `"power"` object in seconds with default fallbacks and range validation
- [x] 1.4 Update shipped hardware configs (`hardware-*.json`) to include `"power"` configuration block in seconds

## Section 2: HardwareInit & 3-State Power Management

- [x] 2.1 Update `HardwareInit::latchPower()` and `HardwareInit::updatePowerButton()` to use `power.bootLatchS` and `power.buttonHoldS`
- [x] 2.2 Add pin guards for `0xFF` on `POWER_ENABLE`, `POWER_BUTTON`, and `CHARGE_SENS` across `HardwareInit`
- [x] 2.3 Add `HardwareInit::isCharging()` charging state detection via `CHARGE_SENS` pin

## Section 3: Disconnect Auto Power-Off & Warning Phase

- [x] 3.1 Implement disconnect auto power-off timer in `VehicleController::update()` using `power.disconnectTimeoutS`
- [x] 3.2 Implement 10-second warning phase (hazard light blink & warning audio alert) when remaining disconnect time is `<= power.warningWindowS`
- [x] 3.3 Implement power button single-click handler in `HardwareInit` / `VehicleController` to reset disconnect timer during warning phase or disconnection
- [x] 3.4 Suspend disconnect auto-off and motor outputs while in `CHARGING` state

## Section 4: Documentation & Config Validation

- [x] 4.1 Update `GUIDE/HARDWARE_CONFIG.md` sections 4.7, 7, and 8 to document the `"power"` block, seconds units, expanded ranges, `CHARGE_SENS` pin, and 3-state board behavior
- [x] 4.2 Run `python3 scripts/validate_configs.py` to confirm all shipped hardware configs and vehicle bundles pass validation

## Section 5: Build & Host Test Verification

- [x] 5.1 Build firmware environments (`MIKRO_V2`, `TRACKLINK_V3`) via `pio run`
- [x] 5.2 Update `test/host_vc/host_vc_driver.cpp` (Test 12) to verify 3-state power control, 10s disconnect warning phase, button click timer reset, and seconds-based config overrides
