# Tasks: Mandatory Drivetrain Configuration

## Section 1: Schema & Firmware Parser Updates

- [x] 1.1 Update `configs/schemas/hardware_config.schema.json`: add `"required": ["drivetrain"]` to top-level schema
- [x] 1.2 Update `common/ConfigParser.h`: add explicit warning diagnostics for missing `drivetrain` section or unconfigured drive motors (`driveMotor` for Ackermann, `leftMotor`/`rightMotor` for skid-steer)

## Section 2: Verification

- [x] 2.1 Run `python3 scripts/validate_configs.py` to confirm all shipped hardware configs pass schema validation
- [x] 2.2 Run `python3 scripts/host_vc_test.py` to verify all host VC unit tests pass
- [x] 2.3 Build firmware environments (`pio run`, `pio run -e MIKRO_V2`) to ensure 0 compilation errors
