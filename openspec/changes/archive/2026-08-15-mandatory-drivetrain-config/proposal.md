# Proposal: Mandatory Drivetrain Configuration & Validation

## Why

The drivetrain / drive motor is essential for an RC vehicle controller. A hardware configuration without a configured drivetrain cannot drive.

To enforce this:
1. `configs/schemas/hardware_config.schema.json` must require `"drivetrain"` as a top-level property.
2. `common/ConfigParser.h` must log explicit warning/error diagnostics if the `drivetrain` section or drive motors (`driveMotor` for Ackermann, or `leftMotor`/`rightMotor` for skid-steer) are missing or unconfigured.

## What Changes

1. **Schema Update**:
   - Add `"required": ["drivetrain"]` to top-level `configs/schemas/hardware_config.schema.json`.

2. **Firmware Parser Updates**:
   - In `common/ConfigParser.h`, log `WARN: drivetrain section missing — drive motor is unconfigured!` if no `drivetrain` section is provided in JSON.
   - For Ackermann mode, log `WARN: drivetrain: drive_motor missing or unconfigured!` if `!config.driveMotor.configured`.
   - For Skid-steer mode, log warnings if `!config.leftMotor.configured` or `!config.rightMotor.configured`.

3. **Verification**:
   - Run `python3 scripts/validate_configs.py`.
   - Run `python3 scripts/host_vc_test.py`.
   - Build firmware environments (`pio run`, `pio run -e MIKRO_V2`).

## Non-goals

- Halting boot on missing drivetrain (firmware continues with warn-and-continue policy as specified in `ConfigParser.h`).
