# Config Filesystem Management

## ADDED Requirements

### Requirement: Lower snake_case JSON schema
The firmware SHALL parse hardware configuration (board-specific: `/hardware-MIKRO_V2.json` or `/hardware-TRACKLINK_V3.json`, selected at compile time by the board define) and vehicle configuration (`/vehicle-config.json`) using `lower_snake_case` JSON keys. The hardware schema SHALL additionally accept an optional `"animation"` block (easing speed/strength, fade duration) whose absence falls back to firmware defaults.

#### Scenario: Parse lower_snake_case hardware config
- **WHEN** the board hardware config containing `sound`, `drivetrain`, `lights`, and `telemetry` keys in `lower_snake_case` is loaded
- **THEN** `ConfigParser` successfully populates `HardwareConfig` with all specified parameters

#### Scenario: Parse lower_snake_case vehicle config
- **WHEN** `/vehicle-config.json` containing `vehicle`, `engine`, `transmission`, and `sound_volumes` keys in `lower_snake_case` is loaded
- **THEN** `ConfigParser` successfully populates `RcEngineSound::Config` with all specified parameters

#### Scenario: Parse optional animation block
- **WHEN** the hardware config contains an `"animation"` block with `easing_speed_deg_s`, `easing_k_in`, `easing_k_out`, and `fade_duration_ms`
- **THEN** `ConfigParser` populates the corresponding `HardwareConfig` animation fields

#### Scenario: Animation block absent uses defaults
- **WHEN** the hardware config omits the `"animation"` block
- **THEN** `ConfigParser` leaves the animation fields at their firmware defaults and loading still succeeds

### Requirement: Deductive drivetrain topology detection
The firmware SHALL automatically deduce the physical drivetrain layout based on the keys present in the `"drivetrain"` JSON object without requiring a mandatory mode tag.

#### Scenario: Skid-steer drivetrain detected
- **WHEN** the `"drivetrain"` object contains both `"left_motor"` and `"right_motor"` keys
- **THEN** `ConfigParser` configures the vehicle controller to operate in Skid-Steer mode (differential mixing of throttle and steering)

#### Scenario: Ackermann drivetrain detected
- **WHEN** the `"drivetrain"` object contains a `"drive_motor"` key (and optional `"steering_servo"`)
- **THEN** `ConfigParser` configures the vehicle controller to operate in standard Ackermann mode (single throttle drive output + steering servo output)
