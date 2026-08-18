## ADDED Requirements

### Requirement: Steering Auto-Cancel Turn Signals
The system SHALL activate turn indicators when steering input exceeds ±35% and automatically cancel them when steering returns to center (|steer| < 10%).

#### Scenario: Steering turned right
- **WHEN** steering input exceeds +35%
- **THEN** the right turn indicator is turned ON

#### Scenario: Steering returned to center
- **WHEN** steering input drops below 10% after an auto-turn-signal was active
- **THEN** the active turn indicator is turned OFF

### Requirement: Dynamic Deceleration Brake Lighting
The system SHALL illuminate brake lights when rapid throttle reduction occurs, even if the brake pedal is not explicitly pressed.

#### Scenario: Rapid throttle drop
- **WHEN** throttle input decreases by more than 30% within a single update cycle
- **THEN** brake lights illuminate for at least 1.5 seconds

### Requirement: Headlight Stepping and Hazard Mode
The system SHALL support 3-state headlight stepping (Off, Low Beam 40%, High Beam 100%) and synchronized dual-indicator hazard flashing.

#### Scenario: Headlight toggle stepped
- **WHEN** the headlight widget is toggled
- **THEN** headlights step sequentially between Off, Low Beam (40% PWM), and High Beam (100% PWM)

#### Scenario: Hazard mode activated
- **WHEN** hazard mode is active
- **THEN** both left and right turn indicators flash synchronously at 1.5 Hz
