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
The system SHALL treat the manual `left_indicator` and `right_indicator` buttons as latched toggle controls (tap to latch on, tap again to release) merged with the steering-based auto turn signals: either source lights its side, and each source releases independently. A manual toggle that is latched SHALL NOT be cancelled by steering returning to center.

#### Scenario: Manual left indicator latched
- **WHEN** the left indicator button is tapped on
- **THEN** the left turn indicator stays lit until the button is tapped off

#### Scenario: Manual and auto merge
- **WHEN** the right indicator button is latched and the steering wheel is then turned right
- **THEN** the right turn indicator remains lit (both sources agree) and stays lit when steering returns to center until the button is released

#### Scenario: Auto signal while manual off
- **WHEN** no manual indicator is latched and steering exceeds the auto-turn threshold
- **THEN** the corresponding side flashes from the auto signal alone

### Requirement: Reversing light automation
The truck reversing light SHALL be illuminated automatically whenever the truck gear is in R (Reverse), in addition to the manual Item E bitmask override. On the locomotive the reversing light remains manual only.

#### Scenario: Gear R lights reversing lamp
- **WHEN** the truck gear is in R and the reversing light is configured
- **THEN** the reversing light is on

#### Scenario: Manual reversing override still works
- **WHEN** the truck light Item E is selected
- **THEN** the reversing light turns on regardless of gear position
