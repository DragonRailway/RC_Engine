## MODIFIED Requirements

### Requirement: Throttle and steering mapping
The firmware SHALL select its input widgets by the configured vehicle type from `/vehicle-config.json` (single source of truth, one model per device), not by the active RadioKit page. For `TRUCK` type the firmware SHALL read `gas_pedal` and `steering_wheel` (plus `truck_light`, `start_button`, `horn_button`, `aux_slider`, `gear_switch`); for `LOCOMOTIVE` type it SHALL read `throttle_slider` (plus `loco_light`, `engine_button`, `bell_button`, `dir_switch`). Motor and servo outputs follow the mapped inputs (provided engine power state is RUNNING and battery voltage is above cutoff threshold).

#### Scenario: Full throttle from gas pedal (truck)
- **WHEN** vehicle type is `TRUCK`, `gas_pedal` reaches its maximum value and engine state is RUNNING and battery is healthy
- **THEN** the drive motor runs at the configured maximum duty (or ESC max pulse) in the configured direction

#### Scenario: Full throttle from loco slider
- **WHEN** vehicle type is `LOCOMOTIVE` and `throttle_slider` reaches its maximum value and engine state is RUNNING and battery is healthy
- **THEN** the drive motor runs at the configured maximum duty in the configured direction

#### Scenario: Gas pedal released
- **WHEN** the truck `gas_pedal` returns to its spring-min value (-100)
- **THEN** the drive motor receives no throttle and the vehicle decelerates toward idle

#### Scenario: Steering right
- **WHEN** `steering_wheel` is turned to the right
- **THEN** the steering servo moves toward the configured right endpoint, proportionally with the knob value

#### Scenario: Steering center
- **WHEN** `steering_wheel` returns to center (0)
- **THEN** the steering servo returns to the configured center position

#### Scenario: Truck widgets ignored on locomotive
- **WHEN** vehicle type is `LOCOMOTIVE`
- **THEN** truck-page widgets (`gas_pedal`, `steering_wheel`, `gear_switch`, `start_button`, `aux_slider`, `horn_button`) have no effect on outputs

### Requirement: Direction and braking
The firmware SHALL apply the truck `gear_switch` (single-select radio: D=0, P=1, R=2) to select truck drive mode and `brake_pedal` to brake the vehicle, and SHALL apply the Loco page `dir_switch` for locomotive forward/reverse. In Drive the motor follows the gas pedal forward; in Park the motor SHALL be locked to zero regardless of throttle and the parking-brake sound SHALL play; in Reverse the motor SHALL run reversed, the reversing beep SHALL play, and the reversing light SHALL be illuminated automatically. A gear change SHALL play the shifting sound only while the engine is RUNNING. Brake blending (proportional, 20% deadband) continues to apply in Drive and Reverse.

#### Scenario: Gear D — drive forward
- **WHEN** the truck gear is in D (index 0) and throttle is applied
- **THEN** the motor runs forward proportional to throttle

#### Scenario: Gear P — park lock
- **WHEN** the truck gear is in P (index 1)
- **THEN** the motor output is forced to zero regardless of throttle and the parking-brake sound plays

#### Scenario: Gear R — reverse
- **WHEN** the truck gear is in R (index 2) and throttle is applied
- **THEN** the motor runs reversed, the reversing beep plays, and the reversing light is illuminated automatically

#### Scenario: Gear change shift sound
- **WHEN** the truck gear changes between D/P/R while the engine is RUNNING
- **THEN** the shifting sound plays; no shift sound plays while the engine is OFF or STARTING

#### Scenario: Direction switch to reverse (locomotive)
- **WHEN** vehicle type is `LOCOMOTIVE` and `dir_switch` is set to its ON position
- **THEN** the drive motor operates in the reverse direction per hardware config

#### Scenario: Brake pedal applied
- **WHEN** `brake_pedal` is pressed beyond a minimum threshold
- **THEN** the drive motor output is reduced/braked proportionally to the pedal position
