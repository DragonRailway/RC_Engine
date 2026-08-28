## ADDED Requirements

### Requirement: Multi-instance drive motors for synchronized drivetrain control
The hardware configuration SHALL support an array of 1 to 2 drive motor objects under `drivetrain.drive_motors`. Each motor object in the array SHALL support independent `hardware` (e.g. `DRIVER_A`, `DRIVER_B`), `frequency`, `direction` (`forward`, `reverse`, `uni_forward`, `uni_reverse`), and `duty` (`min`, `max`) settings. `HardwareInit::setMotor(speed)` SHALL command all configured drive motor channels concurrently with their individual direction and duty mappings.

#### Scenario: Dual-bogie locomotive with mirrored rear motor
- **WHEN** a hardware config declares `drivetrain.drive_motors` with `DRIVER_A` (forward) and `DRIVER_B` (reverse)
- **THEN** both motors are driven simultaneously when speed is commanded, with DRIVER_B applying reversed polarity to match train forward motion

#### Scenario: Primary motor BEMF speed sensing
- **WHEN** multiple drive motors are configured in `drivetrain.drive_motors`
- **THEN** the firmware SHALL designate the first driver (`drive_motors[0]`) as the primary BEMF speed feedback channel

### Requirement: Multi-instance steering servos
The hardware configuration SHALL support an array of 1 to 2 steering servo objects under `drivetrain.steering_servos`. Each servo object SHALL support independent `hardware` (e.g. `S1`, `S2`), `frequency`, and `endpoints` (`left`, `right`, `center`). `HardwareInit::setServo(pos)` SHALL command all configured steering servos concurrently according to their respective endpoint ranges.

#### Scenario: Dual steering servos for 4-wheel or split steering
- **WHEN** a hardware config declares `drivetrain.steering_servos` with `S1` and `S2` having independent endpoint ranges
- **THEN** commanding a steering position writes the appropriate pulse microseconds to both `S1` and `S2`

### Requirement: Multi-pin binding for light channels
The hardware configuration for each light in `lights` SHALL define `hardware` as an array of 1 to 4 pin name strings (e.g. `["L1", "L2"]`). The firmware SHALL initialize all specified pins and command them in unison for on/off states, PWM brightness, fading transitions, and blinking animations.

#### Scenario: Paired headlights on dual LED pins
- **WHEN** `lights.head_light` specifies `hardware: ["L1", "L2"]` with `brightness_max: 60`
- **THEN** both pins `L1` and `L2` are driven to 60% brightness when headlights are activated

### Requirement: Multi-instance aux motors
The hardware configuration SHALL support an array of 1 to 2 aux motor objects under `aux_motors`. Each aux motor SHALL define its `hardware`, `type` (`mixer`, `tipper`, `trailer_dcc`), `frequency`, `direction`, and `duty` limits.

#### Scenario: Dual aux motor configurations
- **WHEN** a hardware config declares an `aux_motors` array with one or more motor entries
- **THEN** the firmware initializes each declared aux channel according to its hardware token (driver or servo) and drive behavior
