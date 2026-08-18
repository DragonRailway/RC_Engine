# Tasks: Unified Configured Flag

## Section 1: C++ Struct & Parser Updates

- [x] 1.1 Update `common/Config.h`: add `bool configured = false;` to `Sound`, `DriveMotor`, `SteeringServo`, `AuxMotor`, `Animation`, `Battery`, and `Power` structs
- [x] 1.2 Update `common/ConfigParser.h`: set `configured = true` when parsing `sound`, `driveMotor`, `leftMotor`, `rightMotor`, `steeringServo`, `auxMotor`, `animation`, `battery`, and `power`

## Section 2: Documentation & Verification

- [x] 2.1 Run `python3 scripts/validate_configs.py` to confirm hardware configs validate cleanly
- [x] 2.2 Update `test/host_vc/host_vc_driver.cpp` and run `python3 scripts/host_vc_test.py` to verify all 13 test suites pass
- [x] 2.3 Build firmware environments (`pio run`, `pio run -e MIKRO_V2`) to ensure 0 compilation errors
