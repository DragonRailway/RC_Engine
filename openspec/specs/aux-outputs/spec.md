## MODIFIED Requirements

### Requirement: Aux motor and aux light in the hardware config
The hardware config SHALL support an `aux_motors` array section (defined like `drive_motors`: `hardware`, `frequency`, `direction`, `duty`) with a `type` field, and an `aux_light` section (defined like `head_light`: `hardware` pin array, `brightness_max`). The `hardware` token SHALL determine the output kind — `DRIVER_*` drives an H-bridge motor, `S*` drives a servo/ESC output, `L*` drives an LED — with no separate `aux_servo` key required.

#### Scenario: Aux motor on a driver output
- **WHEN** a hardware config declares `aux_motors` with `hardware: "DRIVER_B"`
- **THEN** an H-bridge motor channel is initialized on the DRIVER_B pins with the configured frequency, direction, and duty limits

#### Scenario: Aux motor on a servo output
- **WHEN** a hardware config declares `aux_motors` with `hardware: "S2"`
- **THEN** a PPM servo/ESC channel is initialized on the S2 pin

#### Scenario: Aux light
- **WHEN** a hardware config declares `aux_light` with `hardware: ["L4"]`
- **THEN** an LED channel is initialized on the L4 pin with the configured brightness
