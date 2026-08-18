## 1. Config layer: explicit drivetrain.type + validation

- [x] 1.1 Add `drivetrain.type` parsing to `common/ConfigParser.h`: read `type` token (`"ackermann"` / `"skid_steer"`, case-insensitive, with legacy `TYPE` key variant); when absent fall back to today's key-presence inference; log `WARN` and fall back to inference on unrecognized values
- [x] 1.2 Add skid validation warnings in `ConfigParser.h` `checkSemantics`: missing `left_motor`/`right_motor` on a skid config warns and leaves that channel unconfigured; `aux_motor` on a skid config warns that it is ignored (right track owns the second motor output)
- [x] 1.3 Add optional `type` enum (`ackermann` | `skid_steer`) to `drivetrain` in `configs/schemas/hardware_config.schema.json`
- [x] 1.4 Run `python3 scripts/validate_configs.py` — all existing hardware configs still validate

## 2. HardwareInit: two motor channels (core fix)

- [x] 2.1 Introduce a `MotorChannel` struct in `common/HardwareInit.h` (type, direction, duty min/max, attached flag, output object pointers) and extract `initChannel(MotorChannel&, const DriveMotor&)` from `initDriveMotor()` — same wiring: H-bridge (`DRIVER_2PWM` vs `DRIVER_1PWM_1DIR` per `pins.dualPwm`, same invert flag) or ESC/servo (`EasyServo::attach` with 40–900 Hz guard)
- [x] 2.2 Extract `setChannel(MotorChannel&, int16_t speed)` from `setMotor()` — identical duty-window, direction, ESC pulse math, and easing behavior; reroute `setMotor()` through the left channel so Ackermann behavior is unchanged
- [x] 2.3 Skid init path: left channel from `left_motor` (→ `driveMotor`/`escServo`), right channel from `right_motor` (→ `auxMotor`/`auxServo`); ackermann path untouched; no aux work-machine init in skid mode
- [x] 2.4 Implement `setSkidMotors(leftSpeed, rightSpeed)` driving both channels (park zero, reverse negation, brake blend already handled in `VehicleController` — verify only)
- [x] 2.5 Extend `stopAll()`/`hotReload()` teardown to end/detach both channels cleanly (right channel reuses `auxMotor`/`auxServo` teardown)

## 3. Deploy tooling + example config

- [x] 3.1 Add `--hardware <variant>` flag to `scripts/build_fs.py`: select `hardware-<BOARD>-<variant>.json` and stage as `/hardware-<BOARD>.json`; default remains the current unique-match behavior
- [x] 3.2 Add `configs/hardware_configs/hardware-MIKRO_V2-skid.json`: `drivetrain.type` = `"skid_steer"`, `left_motor` on `DRIVER_A`, `right_motor` on `DRIVER_B`, `steering_sensitivity` 80, no `aux_motor`; verify with `build_fs.py --board MIKRO_V2 --hardware skid --dry-run`

## 4. Host tests

- [x] 4.1 Extend `test/host_vc/host_easykit_stubs.cpp` to record writes to the right channel (aux motor / aux ESC)
- [x] 4.2 Add skid tests to `test/host_vc/host_vc_driver.cpp`: straight throttle drives both channels; steering splits them by sensitivity; reverse negates both; park zeroes both; brake blend scales both; `aux_motor` in skid mode leaves aux unconfigured
- [x] 4.3 Run `python3 scripts/host_vc_test.py` — all assertions pass

## 5. Documentation + final checks

- [x] 5.1 Update `GUIDE/HARDWARE_CONFIG.md`: `drivetrain.type` (default inferred), skid-steer layout (`left_motor`/`right_motor`/`steering_sensitivity`), and the `aux_motor` exclusion in skid mode
- [x] 5.2 Final verification: `python3 scripts/validate_configs.py`, `python3 scripts/host_vc_test.py`, `pio run` (both envs compile), and `build_fs.py --dry-run` for an ackermann board and the skid variant
