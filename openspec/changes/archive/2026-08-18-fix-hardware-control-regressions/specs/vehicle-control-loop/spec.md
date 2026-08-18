## MODIFIED Requirements

### Requirement: Throttle and steering mapping
The firmware SHALL select its input widgets by the configured vehicle type from `/vehicle-config.json` (single source of truth, one model per device), not by the active RadioKit page. For `TRUCK` type the firmware SHALL read `gas_pedal` and `steering_wheel` (plus `truck_light`, `start_button`, `horn_button`, `aux_slider`, `gear_switch`); for `LOCOMOTIVE` type it SHALL read `throttle_slider` (plus `loco_light`, `engine_button`, `bell_button`, `dir_switch`).

The firmware SHALL map `gas_pedal` (`[-100, +100]`) to `[0, 100]%` throttle via `(val + 100) / 2` when in truck mode, and read `throttle_slider` (`[0, 100]`) directly when in locomotive mode. Drive motor output follows the mapped inputs provided engine power state is RUNNING and battery voltage is above cutoff threshold. For Ackermann steering setups, the steering servo SHALL follow `steering_wheel` continuously in all active states (including engine OFF, STARTING, and in Park), except when battery voltage drops below cutoff threshold.

The vehicle type SHALL be represented by a single canonical enum (`VEHICLE_TRUCK`, `VEHICLE_LOCOMOTIVE`, `VEHICLE_EXCAVATOR`, `VEHICLE_UNKNOWN`) shared by the config parser and the control loop. An unrecognized `type` string in the vehicle config SHALL map to `VEHICLE_UNKNOWN` and log a boot warning, and SHALL behave as TRUCK for widget selection.

#### Scenario: Full throttle from gas pedal (truck)
- **WHEN** vehicle type is `TRUCK`, `gas_pedal` reaches +100 and engine state is RUNNING and battery is healthy
- **THEN** the drive motor runs at the configured maximum duty (or ESC max pulse) in the configured direction

#### Scenario: Half throttle from gas pedal (truck)
- **WHEN** vehicle type is `TRUCK`, `gas_pedal` is at 0 (50% travel between -100 and +100)
- **THEN** the drive motor throttle percent is computed as 50%

#### Scenario: Gas pedal released
- **WHEN** the truck `gas_pedal` returns to its spring-min value (-100)
- **THEN** the drive motor receives no throttle (0%) and the vehicle decelerates toward idle

#### Scenario: Full throttle from loco slider
- **WHEN** vehicle type is `LOCOMOTIVE` and `throttle_slider` reaches its maximum value and engine state is RUNNING and battery is healthy
- **THEN** the drive motor runs at the configured maximum duty in the configured direction

#### Scenario: Unknown type defaults to truck
- **WHEN** the vehicle config `type` string is not `TRUCK`, `LOCOMOTIVE`, or `EXCAVATOR`
- **THEN** the type resolves to `VEHICLE_UNKNOWN`, a boot warning is logged, and the truck widget set is used

#### Scenario: Excavator type is a recognized stub
- **WHEN** the vehicle config `type` is `EXCAVATOR`
- **THEN** the type resolves to `VEHICLE_EXCAVATOR`, a boot log notes the control surface is deferred, and the truck widget set is used until the excavator surface is implemented

#### Scenario: Steering right while engine is stopped or parked
- **WHEN** `steering_wheel` is turned to the right while engine state is OFF or in Park (P)
- **THEN** the steering servo moves toward the configured right endpoint, proportionally with the knob value

#### Scenario: Steering center
- **WHEN** `steering_wheel` returns to center (0)
- **THEN** the steering servo returns to the configured center position

#### Scenario: Truck widgets ignored on locomotive
- **WHEN** vehicle type is `LOCOMOTIVE`
- **THEN** truck-page widgets (`gas_pedal`, `steering_wheel`, `gear_switch`, `start_button`, `aux_slider`, `horn_button`) have no effect on outputs

### Requirement: Direction and braking
The firmware SHALL apply the truck `gear_switch` (single-select radio: D=0, P=1, R=2) to select truck drive mode and `brake_pedal` to brake the vehicle, and SHALL apply the Loco page `dir_switch` for locomotive forward/reverse.

`brake_pedal` (`[-100, +100]`) SHALL be normalized to `[0, 100]%` via `(val + 100) / 2`. In Drive the motor follows the gas pedal forward; in Park the motor SHALL be locked to zero regardless of throttle and the parking-brake sound SHALL play; in Reverse the motor SHALL run reversed, the reversing beep SHALL play, and the reversing light SHALL be illuminated automatically. A gear change SHALL play the shifting sound only while the engine is RUNNING. When normalized `brake_pedal` is pressed beyond a 20% deadband, the drive motor output SHALL be reduced linearly and proportionally to the pedal position (zero motor output at full brake), applied to both Ackermann and skid-steer drivetrains, while the sound engine RPM simulation continues to follow the raw throttle input.

The firmware SHALL interlock engine start/stop with gear selection: stopping the engine SHALL automatically shift `gear_switch` to Park (`P` / value 1) and sync the widget state to the app. Starting the engine while in Park (`P`) SHALL automatically shift `gear_switch` to Drive (`D` / value 0) and sync the widget state to the app.

#### Scenario: Gear D — drive forward
- **WHEN** the truck gear is in D (index 0) and throttle is applied
- **THEN** the motor runs forward proportional to throttle

#### Scenario: Gear P — park lock
- **WHEN** the truck gear is in P (index 1)
- **THEN** the motor output is forced to zero regardless of throttle and the parking-brake sound plays

#### Scenario: Gear R — reverse
- **WHEN** the truck gear is in R (index 2) and throttle is applied
- **THEN** the motor runs reversed, the reversing beep plays, and the reversing light is illuminated automatically

#### Scenario: Brake pedal released
- **WHEN** `brake_pedal` is at its resting spring position (-100)
- **THEN** normalized brake percentage is 0% and does not impede motor throttle

#### Scenario: Engine stop auto-shifts to Park
- **WHEN** the engine is stopped (latched power button toggles OFF or engine stops)
- **THEN** `gear_switch` automatically switches to Park (`P` / index 1)

#### Scenario: Engine start auto-shifts to Drive
- **WHEN** the engine transitions from stopped/Park to STARTING
- **THEN** `gear_switch` automatically switches to Drive (`D` / index 0)
