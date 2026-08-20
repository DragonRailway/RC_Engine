# aux-outputs Specification

## Purpose
Config-driven auxiliary outputs (mixer, tipper) for truck auxiliaries — dump-truck tipper, cement-mixer drum, and future trailer control — replacing the hardcoded S2/S3 servo attachment with hardware-config-declared aux motor and aux light channels.
## Requirements
### Requirement: Aux motor and aux light in the hardware config
The hardware config SHALL support an `aux_motor` section (defined like
`drive_motor`: `hardware`, `frequency`, `direction`, `duty`) with a `type`
field, and an `aux_light` section (defined like `head_light`: `hardware`,
`brightness_max`). The `hardware` token SHALL determine the output kind —
`DRIVER_*` drives an H-bridge motor, `S*` drives a servo/ESC output, `L*`
drives an LED — with no separate `aux_servo` key required.

#### Scenario: Aux motor on a driver output
- **WHEN** a hardware config declares `aux_motor` with `hardware: "DRIVER_B"`
- **THEN** an H-bridge motor channel is initialized on the DRIVER_B pins with
  the configured frequency, direction, and duty limits

#### Scenario: Aux motor on a servo output
- **WHEN** a hardware config declares `aux_motor` with `hardware: "S2"`
- **THEN** a PPM servo/ESC channel is initialized on the S2 pin

#### Scenario: Aux light
- **WHEN** a hardware config declares `aux_light` with `hardware: "L4"`
- **THEN** an LED channel is initialized on the L4 pin with the configured
  brightness

### Requirement: Mixer and tipper aux behavior via the aux slider
The `aux_motor.type` field SHALL select the aux drive behavior, driven by the
app's `aux_slider`. For `mixer`, the slider SHALL be configured with 5
detents and no self-centering so the user sets a speed/direction that keeps
running. For `tipper`, the slider SHALL be self-centering with no detents so
the position is momentary. The firmware SHALL set the slider's detents and
centering at runtime from the configured type.

#### Scenario: Mixer slider profile
- **WHEN** `aux_motor.type` is `mixer`
- **THEN** the aux slider reports 5 detents and no spring return, and the aux
  motor runs proportionally to the slider position including direction

#### Scenario: Tipper slider profile
- **WHEN** `aux_motor.type` is `tipper`
- **THEN** the aux slider is self-centering with continuous travel, and the
  aux motor follows the momentary position

### Requirement: No legacy auto-attached aux servos
The firmware SHALL NOT auto-attach aux servo outputs to S2/S3 (or any pin)
without a config declaration. Aux outputs SHALL exist only when declared in
the hardware config, and the unused `aux_hydraulic2` channel SHALL be
removed.

#### Scenario: Board without aux config
- **WHEN** a hardware config declares no `aux_motor` or `aux_light`
- **THEN** no aux output channel is initialized, and no pin is driven

### Requirement: trailer_dcc type deferred
The `aux_motor.type` enum SHALL reserve `trailer_dcc`, but a config using it
SHALL be treated as not-yet-implemented: the channel SHALL remain
unconfigured and a warning SHALL be emitted, until a later change implements
the DCC-like trailer control protocol.

#### Scenario: trailer_dcc config warns and degrades
- **WHEN** a hardware config declares `aux_motor` with `type: "trailer_dcc"`
- **THEN** the firmware logs a warning that trailer_dcc is not yet
  implemented and the aux channel is not driven

### Requirement: Vehicle config remains universal
The vehicle config SHALL NOT carry aux pin or channel information; aux
behavior stays keyed off aux activity generically (e.g. hydraulic flow sound
on slider activity), and the hardware config SHALL be the sole source of aux
wiring and purpose.

#### Scenario: Same vehicle config on different boards
- **WHEN** the same `vehicle.json` is deployed with two hardware configs that
  declare different aux wiring
- **THEN** the vehicle config requires no modification and aux behavior is
  identical

