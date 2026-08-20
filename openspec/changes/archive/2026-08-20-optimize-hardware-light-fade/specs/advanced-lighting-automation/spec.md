## MODIFIED Requirements

### Requirement: Headlight Stepping and Hazard Mode
The system SHALL support 3-state headlight stepping (Off, Low Beam 40%, High Beam 100%) with immediate transition (<30ms) and synchronized dual-indicator hazard flashing at the config-defined turn-light interval.

#### Scenario: Headlight toggle stepped
- **WHEN** the headlight widget is toggled
- **THEN** headlights step sequentially between Off, Low Beam (40% PWM), and High Beam (100% PWM) without perceptible fading lag (fade_duration_ms <= 30ms)
