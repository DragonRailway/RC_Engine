## MODIFIED Requirements

### Requirement: Mandatory drivetrain property in hardware configuration schema

The hardware configuration schema (`configs/schemas/hardware_config.schema.json`) SHALL require `"drivetrain"` as a top-level property, containing a mandatory `"drive_motors"` array in Ackermann mode or `"left_motor"` / `"right_motor"` in Skid-steer mode. The schema SHALL also support an optional `"auto_centering"` object with properties `"enabled"` (boolean), `"base_rate"` (number), `"speed_rate"` (number), `"max_rate"` (number), and `"hold_in_reverse"` (boolean).

#### Scenario: Hardware config missing drivetrain section
- **WHEN** a hardware config JSON omits the top-level `"drivetrain"` property
- **THEN** schema validation (`scripts/validate_configs.py`) SHALL fail validation

#### Scenario: Drivetrain contains valid auto_centering block
- **WHEN** a hardware config JSON specifies `"auto_centering": { "enabled": true, "base_rate": 0.0, "speed_rate": 1.5, "max_rate": 8.0, "hold_in_reverse": true }`
- **THEN** schema validation SHALL pass

### Requirement: Config parser validation for missing or unconfigured drive motors

`ConfigParser` in `common/ConfigParser.h` SHALL validate that the drivetrain section and its required drive motors are present and configured:
- If `"drivetrain"` section is missing, log `WARN: drivetrain section missing — drive motor is unconfigured!`
- In Ackermann mode, if `config.driveMotorCount == 0`, log `WARN: drivetrain: drive_motors missing or empty — vehicle cannot drive!`
- In Skid-steer mode, if `!config.leftMotor.configured` or `!config.rightMotor.configured`, log warnings for the unconfigured track motor(s).
- Parse `"auto_centering"` if present, or assign safe defaults if omitted (`enabled = false`, `base_rate = 0.0`, `speed_rate = 1.5`, `max_rate = 8.0`, `hold_in_reverse = true`).

#### Scenario: Missing drive_motors in Ackermann mode
- **WHEN** hardware JSON contains `"drivetrain": { "type": "ackermann" }` without `"drive_motors"` or with an empty array
- **THEN** `ConfigParser` SHALL log `WARN: drivetrain: drive_motors missing or empty — vehicle cannot drive!` and leave `config.driveMotorCount = 0`

#### Scenario: Parse auto_centering configuration
- **WHEN** hardware JSON includes `"auto_centering"` properties
- **THEN** `ConfigParser` populates `config.drivetrain.autoCentering` struct fields with parsed or default values
