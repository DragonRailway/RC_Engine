## ADDED Requirements

### Requirement: Dynamic steering auto-centering calculation
The vehicle controller SHALL calculate a speed-dependent steering return-to-center rate when dynamic auto-centering is enabled and the user is not actively interacting with the steering control. The rate SHALL scale proportionally with forward vehicle speed and decay the effective steering angle smoothly towards 0.

#### Scenario: Steering decays towards center when moving forward
- **WHEN** dynamic auto-centering is enabled, vehicle is moving forward with positive motor speed, and no steering input is received from the user
- **THEN** the steering value decays towards 0 at a rate proportional to forward motor speed

#### Scenario: Steering holds position when stationary
- **WHEN** dynamic auto-centering is enabled with `base_rate = 0.0`, vehicle motor speed is 0, and user releases steering
- **THEN** the steering value remains at its current turned angle without decaying

#### Scenario: Reverse gear hold behavior
- **WHEN** dynamic auto-centering is enabled with `hold_in_reverse = true` and the vehicle is in reverse gear
- **THEN** speed-dependent centering rate is disabled and only `base_rate` decay (if non-zero) applies

### Requirement: User interaction priority
The vehicle controller SHALL immediately yield steering control to user inputs whenever active touch or drag input is detected on the steering control, suspending auto-centering until input ceases.

#### Scenario: User drags steering wheel
- **WHEN** incoming steering packets are received from the RadioKit app within the active touch window
- **THEN** the effective steering angle follows the user's commanded input without auto-centering decay

### Requirement: Bidirectional UI synchronization
When the steering angle is decayed by auto-centering, the firmware SHALL update `steering_wheel.rk.value` to match the decayed integer position, synchronizing the visual state of the steering wheel in the connected mobile app.

#### Scenario: UI steering wheel unwinds to center
- **WHEN** auto-centering decays the steering angle on a moving vehicle
- **THEN** `steering_wheel.rk.value` is updated and dispatched to the RadioKit app via `RK_CMD_SET_INPUT`

### Requirement: Drivetrain mode support
Dynamic auto-centering SHALL apply across all steered drivetrain modes:
- In Ackermann mode, decayed steering controls the physical steering servos.
- In Skid-Steer mode, decayed steering adjusts the differential track/wheel speed offset back towards equal linear drive speed.

#### Scenario: Skid-steer straight line recovery
- **WHEN** a skid-steer vehicle is driven forward and steering input is released
- **THEN** differential track offset decays to 0, returning both left and right tracks to identical drive speeds
