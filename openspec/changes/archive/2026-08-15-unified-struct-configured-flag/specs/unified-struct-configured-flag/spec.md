# Spec: Unified Configured Flag Across All HardwareConfig Structs

## ADDED Requirements

### Requirement: Uniform configured flag for all HardwareConfig sub-structs

Every sub-struct inside `HardwareConfig` (`Sound`, `DriveMotor`, `SteeringServo`, `AuxMotor`, `Light`, `TurnLight`, `DitchLight`, `AuxLight`, `Animation`, `Battery`, `Power`, `Charging`) SHALL contain a `bool configured = false;` field initialized to `false`.

#### Scenario: Section present in JSON
- **WHEN** a hardware config JSON contains a top-level section (e.g. `"power"`, `"battery"`, `"sound"`, `"animation"`)
- **THEN** `ConfigParser` parses the section and sets `config.<section>.configured = true`

#### Scenario: Section omitted from JSON
- **WHEN** a hardware config JSON omits a top-level section
- **THEN** `config.<section>.configured` remains `false`
