## ADDED Requirements

### Requirement: Throttle and steering mapping
The firmware SHALL select its input widgets by the configured vehicle type from `/vehicle-config.json` (single source of truth, one model per device), not by the active RadioKit page. For `TRUCK` type the firmware SHALL read `gas_pedal` and `steering_wheel` (plus `truck_light`, `start_button`, `horn_button`, `aux_slider`, `gear_switch`); for `LOCOMOTIVE` type it SHALL read `throttle_slider` (plus `loco_light`, `engine_button`, `bell_button`, `dir_switch`). Motor and servo outputs follow the mapped inputs (provided engine power state is RUNNING and battery voltage is above cutoff threshold).

The vehicle type SHALL be represented by a single canonical enum (`VEHICLE_TRUCK`, `VEHICLE_LOCOMOTIVE`, `VEHICLE_EXCAVATOR`, `VEHICLE_UNKNOWN`) shared by the config parser and the control loop. An unrecognized `type` string in the vehicle config SHALL map to `VEHICLE_UNKNOWN` and log a boot warning, and SHALL behave as TRUCK for widget selection.

#### Scenario: Full throttle from gas pedal (truck)
- **WHEN** vehicle type is `TRUCK`, `gas_pedal` reaches its maximum value and engine state is RUNNING and battery is healthy
- **THEN** the drive motor runs at the configured maximum duty (or ESC max pulse) in the configured direction

#### Scenario: Full throttle from loco slider
- **WHEN** vehicle type is `LOCOMOTIVE` and `throttle_slider` reaches its maximum value and engine state is RUNNING and battery is healthy
- **THEN** the drive motor runs at the configured maximum duty in the configured direction

#### Scenario: Unknown type defaults to truck
- **WHEN** the vehicle config `type` string is not `TRUCK`, `LOCOMOTIVE`, or `EXCAVATOR`
- **THEN** the type resolves to `VEHICLE_UNKNOWN`, a boot warning is logged, and the truck widget set is used

#### Scenario: Excavator type is a recognized stub
- **WHEN** the vehicle config `type` is `EXCAVATOR`
- **THEN** the type resolves to `VEHICLE_EXCAVATOR`, a boot log notes the control surface is deferred, and the truck widget set is used until the excavator surface is implemented

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
The firmware SHALL apply the truck `gear_switch` (single-select radio: D=0, P=1, R=2) to select truck drive mode and `brake_pedal` to brake the vehicle, and SHALL apply the Loco page `dir_switch` for locomotive forward/reverse. In Drive the motor follows the gas pedal forward; in Park the motor SHALL be locked to zero regardless of throttle and the parking-brake sound SHALL play; in Reverse the motor SHALL run reversed, the reversing beep SHALL play, and the reversing light SHALL be illuminated automatically. A gear change SHALL play the shifting sound only while the engine is RUNNING. When `brake_pedal` is pressed beyond a 20% deadband, the drive motor output SHALL be reduced linearly and proportionally to the pedal position (zero motor output at full brake), applied to both Ackermann and skid-steer drivetrains, while the sound engine RPM simulation continues to follow the raw throttle input.

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

#### Scenario: Brake overrides throttle
- **WHEN** throttle is applied while `brake_pedal` is pressed
- **THEN** the motor output equals the throttle scaled by the remaining brake headroom (full brake → zero motor output)

#### Scenario: Brake does not stall the engine simulation
- **WHEN** `brake_pedal` is pressed while throttle is released
- **THEN** motor output is zero and the sound engine RPM continues to follow the raw throttle (idle) with the brake sound effect active

### Requirement: Light control
The firmware SHALL map the `truck_light` (Truck page) and `loco_light` (Loco page) multi-select bitmasks, as well as automatic steering indicators, dynamic deceleration brake lights, and low-battery hazard overrides, to the configured light outputs (head, tail, brake, turn, reversing).

#### Scenario: LED group selected
- **WHEN** an item (A/B/C) is selected on `truck_light` or `loco_light`
- **THEN** the corresponding configured light outputs are enabled via PWM

#### Scenario: All lights deselected
- **WHEN** no items are selected on a light widget and no automation is active
- **THEN** the corresponding light outputs are disabled

### Requirement: Horn and sound engine drive
The firmware SHALL trigger the horn sound while `horn_button` is pressed (truck) or the bell sound while `bell_button` is pressed (locomotive), and SHALL feed throttle and hydraulic governor pump load into the sound engine so engine RPM and pitch follow gas pedal, Loco slider, and active hydraulic flow inputs.

#### Scenario: Horn pressed
- **WHEN** `horn_button` is pressed on a truck
- **THEN** the sound engine plays the horn sound at its configured volume

#### Scenario: Bell pressed
- **WHEN** `bell_button` is pressed on a locomotive
- **THEN** the sound engine plays the bell sound at its configured volume

#### Scenario: Throttle raises engine RPM
- **WHEN** the gas pedal, Loco slider, or hydraulic flow demand increases
- **THEN** the sound engine's simulated RPM rises and sound pitch follows

### Requirement: Telemetry reporting
The firmware SHALL update `telemetry_Battery` with battery voltage converted to percent and `telemetry_Speed` with an estimated speed derived from throttle, at a bounded rate. Battery voltage SHALL be computed using `voltage_scale` / `voltage_offset` from `hardware-config.json` when present, falling back to the board's compile-time `VSCALE` / `VOFFSET` calibration values when absent.

#### Scenario: Battery telemetry updated
- **WHEN** the control loop runs
- **THEN** `telemetry_Battery.rk.content` holds a percentage string based on the measured voltage

#### Scenario: Speed telemetry updated
- **WHEN** the control loop runs
- **THEN** `telemetry_Speed.rk.content` holds a speed estimate derived from throttle and direction

#### Scenario: Config calibration applied
- **WHEN** `hardware-config.json` contains `voltage_scale` and `voltage_offset`
- **THEN** battery percentage is computed from the configured calibration values

#### Scenario: Config calibration absent
- **WHEN** `hardware-config.json` does not contain `voltage_scale` or `voltage_offset`
- **THEN** battery percentage is computed using the board's compile-time `VSCALE` / `VOFFSET` calibration
