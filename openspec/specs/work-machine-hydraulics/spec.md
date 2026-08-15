# Work Machine Hydraulics Specification

## Purpose
Defines requirements for hydraulic flow sounds, load governor RPM bumps, track rattle, and auxiliary actuator controls.

## Requirements

### Requirement: Hydraulic Flow Sound and Load Governor
The system SHALL detect auxiliary hydraulic control activity (> 10% magnitude), trigger hydraulic flow sound hiss, and increase engine target RPM by +20% of maximum RPM to simulate hydraulic pump load.

#### Scenario: Hydraulic movement detected
- **WHEN** auxiliary hydraulic control input magnitude exceeds 10%
- **THEN** hydraulic flow sound hiss triggers and engine target RPM is increased by 20% of max RPM

#### Scenario: Hydraulic movement idle
- **WHEN** auxiliary hydraulic control inputs return below 10%
- **THEN** hydraulic flow sound hiss deactivates and engine RPM returns to base throttle level

### Requirement: Track Pin Rattle Synthesis
The system SHALL evaluate vehicle movement speed when track rattle is enabled and trigger rhythmic metallic pin clanking sound effects with interval scaling based on speed.

#### Scenario: Tracked vehicle in motion
- **WHEN** vehicle speed is greater than zero and track rattle is enabled
- **THEN** track pin rattle sound triggers with clank intervals scaling from 500ms at low speed down to 90ms at top speed

### Requirement: Config-driven auxiliary outputs
The system SHALL map auxiliary hydraulic control values (provided through the public input surface) to the configured aux output channel. Aux outputs are declared in the **hardware config** (`aux_motor` / `aux_light`), not hardcoded to pins: the `hardware` token decides the output kind (`DRIVER_*` → H-bridge via `EasyMotor`, `S*` → servo/ESC PPM via `EasyServo`, `L*` → LED via `EasyLED`), and `aux_motor.type` selects the control profile (mixer: proportional incl. direction; tipper: momentary). The former hardcoded S2/S3 auto-attach is removed — an undeclared aux config initializes no channel.

#### Scenario: Auxiliary positioning on a configured channel
- **WHEN** auxiliary hydraulic control values change on a config with `aux_motor` declared
- **THEN** the configured aux motor channel adjusts output proportionally (PPM pulse width for `S*`, H-bridge duty for `DRIVER_*`)

#### Scenario: Board without aux config
- **WHEN** the active board's hardware config declares no `aux_motor` / `aux_light`
- **THEN** no aux output channel is initialized and no pin is driven

### Requirement: Auxiliary hydraulic input surface
The firmware SHALL expose the work-machine hydraulic channel values, dump bed toggle, and bucket rattle trigger as writable public fields so external UI wiring (RadioKit widget bindings) can drive work-machine behavior without firmware-side input mapping. The UI bindings themselves are out of scope for the firmware.

#### Scenario: UI writes a hydraulic value
- **WHEN** external UI wiring writes a hydraulic channel value with magnitude above 10%
- **THEN** the control loop applies the corresponding aux output, activates hydraulic flow sound, and applies the engine load governor

#### Scenario: UI triggers dump bed
- **WHEN** external UI wiring sets the dump bed toggle
- **THEN** the dump bed sound effect activates and the hydraulic flow governor is applied
