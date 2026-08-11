## MODIFIED Requirements

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
