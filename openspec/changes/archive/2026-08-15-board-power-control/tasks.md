# Tasks

## Section 1: Schema & Config Parser

- [x] 1.1 Update `configs/schemas/hardware_config.schema.json`: add `warning_voltage` (default 3.5V, range 3.0–4.0V) and set default `cutoff_voltage` to 3.3V (range 3.0–3.8V)

- [x] 1.2 Update `common/Config.h`: add `warningVoltage` to `HardwareConfig::Battery` struct

- [x] 1.3 Update `common/ConfigParser.h`: parse `warning_voltage` (default 3.5f) and update default `cutoff_voltage` to 3.3f

- [x] 1.4 Update shipped hardware configs (`hardware-*.json`) to include `warning_voltage: 3.5` and `cutoff_voltage: 3.3`

## Section 2: Power Latching & Control in HardwareInit

- [x] 2.1 Add `HardwareInit::latchPower()`, `HardwareInit::updatePowerButton()`, and `HardwareInit::powerOff()` in `common/HardwareInit.h`
- [x] 2.2 Implement 1000ms boot power-on latch filter in `HardwareInit::latchPower()`
- [x] 2.3 Implement 4000ms button hold detection in `HardwareInit::updatePowerButton()` triggering `HardwareInit::powerOff()`

## Section 3: Integration in Control Loop & Battery Safety

- [x] 3.1 Call `HardwareInit::latchPower()` at the beginning of `setup()` in `src/main.cpp`
- [x] 3.2 Update `VehicleController::update()`: implement two-tier battery warning (3.5V/cell alert) and cutoff (3.3V/cell 1500ms delay → `HardwareInit::powerOff()`)
- [x] 3.3 Add `HardwareInit::updatePowerButton()` to main control pump in `src/main.cpp` / `HardwareInit::update()`

## Section 4: Documentation & Config Validation

- [x] 4.1 Update `GUIDE/HARDWARE_CONFIG.md` section 4.7 (Battery configuration) with `warning_voltage` and updated `cutoff_voltage` (3.3V)
- [x] 4.2 Run `python3 scripts/validate_configs.py` to confirm all shipped hardware configs and vehicle bundles pass validation

## Section 5: Build & Test Verification

- [x] 5.1 Build firmware environments (`MIKRO_V2`, `TRACKLINK_V3`) via `pio run`
- [x] 5.2 Add Test 11 to `test/host_vc/host_vc_driver.cpp` to verify 1000ms latch filter, 4000ms hold shutdown, and two-tier battery protection
- [x] 5.3 Bench/hardware verification: flash MIKRO_V2 board and verify 1s boot hold latch, 4s hold power-off, and low-voltage cutoff
