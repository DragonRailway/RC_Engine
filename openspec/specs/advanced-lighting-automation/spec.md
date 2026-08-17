# Advanced Lighting Automation Specification

## Purpose
Defines requirements for automated lighting behaviors including steering auto turn signals, dynamic deceleration brake lights, and 3-state headlight stepping.

## Requirements

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
The system SHALL support 3-state headlight stepping (Off, Low Beam 40%, High Beam 100%) and synchronized dual-indicator hazard flashing at the config-defined turn-light interval.

#### Scenario: Headlight toggle stepped
- **WHEN** the headlight widget is toggled
- **THEN** headlights step sequentially between Off, Low Beam (40% PWM), and High Beam (100% PWM)

#### Scenario: Hazard mode activated
- **WHEN** hazard mode is active
- **THEN** both left and right turn indicators flash synchronously at the interval defined by `turn_light.interval_on`/`interval_off` in the hardware config

### Requirement: Indicator click sound
The system SHALL play the indicator click sound while any turn indicator is active — whether from automatic steering-based turn signals or manual hazard flashing.

#### Scenario: Auto turn signal sounds
- **WHEN** a steering-based auto turn signal is active
- **THEN** the indicator click sound plays

#### Scenario: Hazard mode sounds
- **WHEN** hazard mode is active
- **THEN** the indicator click sound plays

#### Scenario: No indicator, no sound
- **WHEN** no turn indicator is active
- **THEN** the indicator click sound is silent

### Requirement: Manual turn signal toggle buttons
The system SHALL treat the manual `left_indicator` and `right_indicator` buttons as toggle controls. Engaging a turn indicator SHALL capture the steering wheel angle at button press as the baseline. The active indicator SHALL automatically cancel (setting the widget state to false and updating the app UI) when:
1. The steering wheel is turned into the indicated direction by at least 20% relative to the baseline (`delta <= -20%` for Left, `delta >= +20%` for Right) and then returns towards the baseline / center by at least 15%.
2. The steering wheel is turned in the opposite direction beyond threshold (+15% relative to the baseline for Left indicator, -15% relative to the baseline for Right indicator).

Engaging an indicator SHALL automatically turn off the opposite indicator.

#### Scenario: Left indicator cancelled on steering return
- **WHEN** `left_indicator` is ON, the steering wheel turns left by >=20% from baseline, and then returns towards center/baseline
- **THEN** `left_indicator` is automatically turned OFF, blinking stops, and UI toggle state resets to false

#### Scenario: Right indicator cancelled on steering return
- **WHEN** `right_indicator` is ON, the steering wheel turns right by >=20% from baseline, and then returns towards center/baseline
- **THEN** `right_indicator` is automatically turned OFF, blinking stops, and UI toggle state resets to false

#### Scenario: Left indicator cancelled on opposite steer
- **WHEN** `left_indicator` is ON and the steering wheel turns right by >15% relative to baseline
- **THEN** `left_indicator` is immediately turned OFF, blinking stops, and UI toggle state resets to false

#### Scenario: Right indicator cancelled on opposite steer
- **WHEN** `right_indicator` is ON and the steering wheel turns left by >15% relative to baseline
- **THEN** `right_indicator` is immediately turned OFF, blinking stops, and UI toggle state resets to false

#### Scenario: Mutual exclusion between indicators
- **WHEN** `left_indicator` is ON and the user activates `right_indicator`
- **THEN** `left_indicator` is set to OFF and `right_indicator` is set to ON

### Requirement: Reversing light automation
The truck reversing light SHALL be illuminated automatically whenever the truck gear is in R (Reverse), in addition to the manual Item E bitmask override. On the locomotive the reversing light remains manual only.

#### Scenario: Gear R lights reversing lamp
- **WHEN** the truck gear is in R and the reversing light is configured
- **THEN** the reversing light is on

#### Scenario: Manual reversing override still works
- **WHEN** the truck light Item E is selected
- **THEN** the reversing light turns on regardless of gear position
