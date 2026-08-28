## MODIFIED Requirements

### Requirement: Mandatory drivetrain property in hardware configuration schema

The hardware configuration schema (`configs/schemas/hardware_config.schema.json`) SHALL require `"drivetrain"` as a top-level property, containing a mandatory `"drive_motors"` array in Ackermann mode or `"left_motor"` / `"right_motor"` in Skid-steer mode.

#### Scenario: Hardware config missing drivetrain section
- **WHEN** a hardware config JSON omits the top-level `"drivetrain"` property
- **THEN** schema validation (`scripts/validate_configs.py`) SHALL fail validation

### Requirement: Config parser validation for missing or unconfigured drive motors

`ConfigParser` in `common/ConfigParser.h` SHALL validate that the drivetrain section and its required drive motors are present and configured:
- If `"drivetrain"` section is missing, log `WARN: drivetrain section missing — drive motor is unconfigured!`
- In Ackermann mode, if `config.driveMotorCount == 0`, log `WARN: drivetrain: drive_motors missing or empty — vehicle cannot drive!`
- In Skid-steer mode, if `!config.leftMotor.configured` or `!config.rightMotor.configured`, log warnings for the unconfigured track motor(s).

#### Scenario: Missing drive_motors in Ackermann mode
- **WHEN** hardware JSON contains `"drivetrain": { "type": "ackermann" }` without `"drive_motors"` or with an empty array
- **THEN** `ConfigParser` SHALL log `WARN: drivetrain: drive_motors missing or empty — vehicle cannot drive!` and leave `config.driveMotorCount = 0`
