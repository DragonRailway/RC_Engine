## MODIFIED Requirements

### Requirement: Direction and braking
The firmware SHALL apply `dir_switch` to select forward/reverse operation and `brake_pedal` to brake the vehicle, respecting the drive motor's configured direction and duty limits. When `brake_pedal` is pressed beyond a 20% deadband, the drive motor output SHALL be reduced linearly and proportionally to the pedal position (zero motor output at full brake), applied to both Ackermann and skid-steer drivetrains, while the sound engine RPM simulation continues to follow the raw throttle input.

#### Scenario: Direction switch to reverse
- **WHEN** `dir_switch` is set to its ON position
- **THEN** the drive motor operates in the reverse direction per hardware config

#### Scenario: Brake pedal applied
- **WHEN** `brake_pedal` is pressed beyond a minimum threshold
- **THEN** the drive motor output is reduced/braked proportionally to the pedal position

#### Scenario: Brake overrides throttle
- **WHEN** throttle is applied while `brake_pedal` is pressed
- **THEN** the motor output equals the throttle scaled by the remaining brake headroom (full brake → zero motor output)

#### Scenario: Brake does not stall the engine simulation
- **WHEN** `brake_pedal` is pressed while throttle is released
- **THEN** motor output is zero and the sound engine RPM continues to follow the raw throttle (idle) with the brake sound effect active
