## ADDED Requirements

### Requirement: Drive Motor PWM and Gear Safety Clamping
The system SHALL control drive motor PWM outputs according to selected gear (Drive, Park, Reverse) and proportional brake blending.

#### Scenario: Park lock zero-torque safety
- **WHEN** Gear P (Park lock, `gear=1`) is selected
- **THEN** motor PWM output SHALL be clamped to 0 regardless of gas pedal input

#### Scenario: Proportional brake blending
- **WHEN** brake pedal input exceeds 20% pressure during acceleration
- **THEN** motor PWM output SHALL attenuate linearly from full speed to zero at 100% brake pressure

#### Scenario: Reverse direction inversion
- **WHEN** Gear R (Reverse, `gear=2`) is selected
- **THEN** motor PWM direction SHALL be inverted and reversing alert voices/lights SHALL activate

### Requirement: Light State Automation
The system SHALL automate 3-state headlights, dynamic deceleration brake lights, and auto-canceling turn signal indicators.

#### Scenario: Headlight 3-state stepping
- **WHEN** headlight control toggle cycles through states
- **THEN** headlight PWM output SHALL step between 0%, 40% (Low Beam), and 100% (High Beam)

#### Scenario: Dynamic deceleration brake light trigger
- **WHEN** throttle drops rapidly from high speed to zero
- **THEN** brake light output SHALL illuminate automatically for the configured deceleration period

#### Scenario: Auto turn signal cancellation
- **WHEN** turn signal is active and steering wheel returns past neutral deadband (±10%)
- **THEN** turn signal indicator SHALL cancel and turn off automatically

### Requirement: LiPo Low-Voltage Cutoff
The system SHALL protect battery packs by enforcing low-voltage cutoff with a 1.5-second debounce filter.

#### Scenario: Battery voltage drop below threshold
- **WHEN** battery voltage remains below cell cutoff threshold for more than 1.5 seconds
- **THEN** motor output SHALL be set to 0, hazard lights SHALL flash at 333ms interval, and out-of-fuel alert sound SHALL trigger
