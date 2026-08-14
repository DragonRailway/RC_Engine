# config-reference-guide Specification

## Purpose

Document the hardware config (`hardware-<BOARD>.json`) for users in a Klipper-style reference guide, covering every config section, parameter defaults, the per-board pin vocabulary, and the quirks and legacy behaviors that the firmware does not validate. The guide exists because the firmware applies no schema validation — it is the de-facto schema for the hardware config.

## Requirements

### Requirement: Hardware config reference documentation
The repository SHALL provide a Klipper-style reference guide at `GUIDE/HARDWARE_CONFIG.md` documenting the hardware config (`hardware-<BOARD>.json`) end-to-end: every config section (`sound`, `drivetrain`, `lights`, `animation`, `battery`) with each parameter's name, type, default value, allowed values, and a description, where defaults match the firmware's `HardwareConfig` struct defaults.

#### Scenario: Looking up a parameter default
- **WHEN** a user consults the reference for `drivetrain.drive_motor.duty.min`
- **THEN** they find its type (integer), default (`20`), allowed range (0–100), and meaning

#### Scenario: Drivetrain fork documented
- **WHEN** a user reads the `drivetrain` section
- **THEN** they learn that presence of `left_motor` selects skid-steer (with `right_motor` + `steering_sensitivity`) while its absence selects Ackermann (`drive_motor` + `steering_servo`), and that mixing the two layouts is not supported

### Requirement: Per-board pin reference
The reference guide SHALL include per-board pin tables for MIKRO_V2 and TRACKLINK_V3 mapping every `hardware` token (`L<n>`, `S<n>`, `DRIVER_A`, `DRIVER_B`) to its physical GPIO and driver pin assignments, and SHALL document the motor-driver tokens as semantic markers resolved per board.

#### Scenario: Choosing a pin across boards
- **WHEN** a user wires a headlight to `L1` and checks the pin reference for their board
- **THEN** they see the correct physical GPIO for their board (GPIO 38 on MIKRO_V2, GPIO 6 on TRACKLINK_V3)

#### Scenario: H-bridge semantic markers
- **WHEN** a user looks up `DRIVER_A`
- **THEN** they find its per-board PWM/DIR/EN/BEMF pin assignments and note that `DRIVER_B` is dual-PWM on MIKRO_V2 but DIR+PWM on TRACKLINK_V3

### Requirement: Quirks and legacy behavior documented
The reference guide SHALL explicitly document parameters that are accepted but ignored, values forced by the firmware, and legacy case variants, and SHALL state in its introduction that the firmware performs no schema validation (unknown keys silently ignored; unrecognized `hardware` tokens result in unconfigured outputs).

#### Scenario: Avoiding a misleading key
- **WHEN** a user configures `lights.turn_light` and encounters the `type` key
- **THEN** the guide notes it is accepted but ignored, and that blinking is driven purely by `interval_on`/`interval_off`

#### Scenario: Forced brightness called out
- **WHEN** a user reads the `lights.brake_light` or `lights.reversing_light` sections
- **THEN** they learn `brightness_max` is not honored and the firmware forces 100% brightness

#### Scenario: Reversing light alias documented
- **WHEN** a user configures `lights.reversing_light`
- **THEN** they learn `hardware` accepts either a pin token or a light alias (`"head_light"`, `"tail_light"`, `"brake_light"`)

### Requirement: Guide index
The repository SHALL provide `GUIDE/README.md` as an index of the guide directory, including a note that a vehicle-config reference is a future stub.

#### Scenario: Opening the guide directory
- **WHEN** a user opens the `GUIDE/` directory
- **THEN** `README.md` links to the hardware config reference and notes the vehicle-config reference is planned but not yet written
