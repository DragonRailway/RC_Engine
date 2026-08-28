## 1. Schema & Configuration Updates

- [x] 1.1 Update `configs/schemas/hardware_config.schema.json` to enforce `drive_motors`, `steering_servos`, `aux_motors`, and array-based `hardware` pins for lights.
- [x] 1.2 Update all 5 hardware configuration files in `configs/hardware_configs/` (`hardware-TRACKLINK_V3-locomotive.json`, `hardware-MIKRO_V2-truck.json`, `hardware-MIKRO_V2-skid.json`, `hardware-GTRACK.json`, `hardware-GTRACK-excavator.json`) to the new schema.
- [x] 1.3 Add a dual-bogie locomotive test configuration using both `DRIVER_A` and `DRIVER_B`.

## 2. Firmware Config Structures & Parser

- [x] 2.1 Update `common/Config.h` to define fixed-size arrays for `driveMotors`, `steeringServos`, `auxMotors`, and multi-pin `Light` structs.
- [x] 2.2 Update `common/ConfigParser.h` to parse canonical array sections for drive motors, steering servos, aux motors, and light pins.
- [x] 2.3 Add config validation warnings for empty or unconfigured motor arrays in `ConfigParser.h`.

## 3. Peripheral Initialization & Control Runtime

- [x] 3.1 Update `common/HardwareInit.h` to initialize and manage arrays of `MotorChannel` and `EasyServo` steering channels.
- [x] 3.2 Update `HardwareInit::setMotor(speed)` to drive all active drive motor channels concurrently with per-channel polarity and duty limits.
- [x] 3.3 Ensure primary driver (`s_driveCh[0]`) is routed to BEMF speed sensing feedback.
- [x] 3.4 Update `HardwareInit::setServo(pos)` to command all configured steering servos across their individual endpoint ranges.
- [x] 3.5 Update `HardwareInit::setLight` and LED animation/blink handlers to drive all physical pins mapped to a light channel.
- [x] 3.6 Update `HardwareInit::stopAll()` and `HardwareInit::setAllMotors(0)` to stop all configured channels.

## 4. Verification & Validation

- [x] 4.1 Run `python3 scripts/validate_configs.py` and verify all hardware configs pass schema validation.
- [x] 4.2 Compile firmware using `pio run` across supported environments to verify clean compilation.
