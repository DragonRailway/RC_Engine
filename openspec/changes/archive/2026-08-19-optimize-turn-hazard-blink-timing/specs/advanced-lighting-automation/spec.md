## MODIFIED Requirements

### Requirement: Headlight Stepping and Hazard Mode
The system SHALL support 3-state headlight stepping (Off, Low Beam 40%, High Beam 100%) and synchronized dual-indicator hazard flashing at the config-defined turn-light interval (defaulting to 300ms ON / 300ms OFF).

#### Scenario: Headlight toggle stepped
- **WHEN** the headlight widget is toggled
- **THEN** headlights step sequentially between Off, Low Beam (40% PWM), and High Beam (100% PWM)

#### Scenario: Hazard mode activated
- **WHEN** hazard mode is active
- **THEN** both left and right turn indicators flash synchronously at the interval defined by `turn_light.interval_on`/`interval_off` in the hardware config (default 300ms ON / 300ms OFF)
