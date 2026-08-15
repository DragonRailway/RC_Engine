# mandatory-drivetrain-config Specification

## Purpose
TBD - created by archiving change mandatory-drivetrain-config. Update Purpose after archive.
## Requirements
### Requirement: Mandatory drivetrain property in hardware configuration schema

The hardware configuration schema (`configs/schemas/hardware_config.schema.json`) SHALL require `"drivetrain"` as a top-level property.

#### Scenario: Hardware config missing drivetrain section
- **WHEN** a hardware config JSON omits the top-level `"drivetrain"` property
- **THEN** schema validation (`scripts/validate_configs.py`) SHALL fail validation

### Requirement: Config parser validation for missing or unconfigured drive motors

`ConfigParser` in `common/ConfigParser.h` SHALL validate that the drivetrain section and its required drive motors are present and configured:
- If `"drivetrain"` section is missing, log `WARN: drivetrain section missing — drive motor is unconfigured!`
- In Ackermann mode, if `!config.driveMotor.configured`, log `WARN: drivetrain: drive_motor missing or unconfigured!`
- In Skid-steer mode, if `!config.leftMotor.configured` or `!config.rightMotor.configured`, log warnings for the unconfigured track motor(s).

#### Scenario: Missing drive_motor in Ackermann mode
- **WHEN** hardware JSON contains `"drivetrain": { "type": "ackermann" }` without `"drive_motor"`
- **THEN** `ConfigParser` SHALL log `WARN: drivetrain: drive_motor missing or unconfigured!` and leave `config.driveMotor.configured = false`

