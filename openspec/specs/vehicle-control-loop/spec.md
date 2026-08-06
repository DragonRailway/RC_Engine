# Vehicle Control Loop

## ADDED Requirements

### Requirement: Throttle and steering mapping
The firmware SHALL map RadioKit input widgets (-100..+100) to motor and servo outputs: gas_pedal (springs to min, released = -100) and the Loco page slider (non-latching) drive the drive motor duty or ESC pulse; steering_wheel (springs to center, 0 = centered) drives the steering servo between its configured left/right endpoints.

#### Scenario: Full throttle from gas pedal
- **WHEN** `gas_pedal` reaches its maximum value
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

### Requirement: Direction and braking
The firmware SHALL apply `dir_switch` to select forward/reverse operation and `brake_pedal` to brake the vehicle, respecting the drive motor's configured direction and duty limits.

#### Scenario: Direction switch to reverse
- **WHEN** `dir_switch` is set to its ON position
- **THEN** the drive motor operates in the reverse direction per hardware config

#### Scenario: Brake pedal applied
- **WHEN** `brake_pedal` is pressed beyond a minimum threshold
- **THEN** the drive motor output is reduced/braked proportionally to the pedal position

### Requirement: Light control
The firmware SHALL map the `led_select` (Truck page) and `lights_toggle` (Loco page) multi-select bitmasks to the configured light outputs (head, tail, brake, turn, reversing).

#### Scenario: LED group selected
- **WHEN** an item (A/B/C) is selected on `led_select` or `lights_toggle`
- **THEN** the corresponding configured light outputs are enabled via PWM

#### Scenario: All lights deselected
- **WHEN** no items are selected on a light widget
- **THEN** the corresponding light outputs are disabled

### Requirement: Horn and sound engine drive
The firmware SHALL trigger the horn sound while `horn` is pressed and SHALL feed throttle into the sound engine so engine RPM and pitch follow the gas pedal and Loco slider inputs.

#### Scenario: Horn pressed
- **WHEN** `horn` is pressed
- **THEN** the sound engine plays the horn sound at its configured volume

#### Scenario: Throttle raises engine RPM
- **WHEN** the gas pedal or Loco slider increases
- **THEN** the sound engine's simulated RPM rises and sound pitch follows

### Requirement: Telemetry reporting
The firmware SHALL update `telemetry_Battery` with battery voltage converted to percent (using the board's VSCALE/VOFFSET calibration) and `telemetry_Speed` with an estimated speed derived from throttle, at a bounded rate.

#### Scenario: Battery telemetry updated
- **WHEN** the control loop runs
- **THEN** `telemetry_Battery.rk.content` holds a percentage string based on the measured voltage

#### Scenario: Speed telemetry updated
- **WHEN** the control loop runs
- **THEN** `telemetry_Speed.rk.content` holds a speed estimate derived from throttle and direction
