## MODIFIED Requirements

### Requirement: Throttle and steering mapping
The firmware SHALL map RadioKit input widgets (-100..+100) to motor and servo outputs: gas_pedal (springs to min, released = -100) and the Loco page slider (non-latching) drive the drive motor duty or ESC pulse (provided engine power state is RUNNING and battery voltage is above cutoff threshold); steering_wheel (springs to center, 0 = centered) drives the steering servo between its configured left/right endpoints.

#### Scenario: Full throttle from gas pedal
- **WHEN** `gas_pedal` reaches its maximum value and engine state is RUNNING and battery is healthy
- **THEN** the drive motor runs at the configured maximum duty (or ESC max pulse) in the configured direction

#### Scenario: Gas pedal released
- **WHEN** `gas_pedal` returns to its spring-min value (-100)
- **THEN** the drive motor receives no throttle and the vehicle decelerates toward idle

#### Scenario: Steering right
- **WHEN** `steering_wheel` is turned to the right
- **THEN** the steering servo moves toward the configured right endpoint, proportionally with the knob value

#### Scenario: Steering center
- **WHEN** `steering_wheel` returns to center (0)
- **THEN** the steering servo returns to the configured center position

### Requirement: Light control
The firmware SHALL map the `led_select` (Truck page) and `lights_toggle` (Loco page) multi-select bitmasks, as well as automatic steering indicators, dynamic deceleration brake lights, and low-battery hazard overrides, to the configured light outputs (head, tail, brake, turn, reversing).

#### Scenario: LED group selected
- **WHEN** an item (A/B/C) is selected on `led_select` or `lights_toggle`
- **THEN** the corresponding configured light outputs are enabled via PWM

#### Scenario: All lights deselected
- **WHEN** no items are selected on a light widget and no automation is active
- **THEN** the corresponding light outputs are disabled
