## ADDED Requirements

### Requirement: Skid-steer differential drive
When the hardware config declares skid-steer (`drivetrain.type` = `"skid_steer"`, or the backward-compatible `left_motor`-presence inference), the firmware SHALL drive two independent motor channels from the configured `left_motor` and `right_motor` outputs, applying the differential mix `left = throttle + steering·sensitivity/100` and `right = throttle − steering·sensitivity/100` (both clamped to the motor speed range), so a steering input splits throttle between the tracks. Reverse SHALL negate both channels, park lock SHALL force both channels to zero regardless of throttle and steering, and brake-pedal blending SHALL scale both channels identically. Each channel SHALL apply its own configured polarity (`direction`), duty window (`duty.min`/`duty.max`), and electrical kind (H-bridge `DRIVER_*` or ESC/servo `S*`). A skid-steer config that omits `left_motor` or `right_motor` SHALL log a `WARN` at boot and leave the affected channel unconfigured.

#### Scenario: Full throttle straight ahead
- **WHEN** the hardware config is skid-steer, gear is D, `gas_pedal` is at maximum, and `steering_wheel` is centered
- **THEN** both the left and right motor channels run forward at the configured maximum duty in their configured directions

#### Scenario: Steering right splits the tracks
- **WHEN** gear is D and the truck `steering_wheel` is turned right while throttle is applied
- **THEN** the left channel receives `throttle + steering·sensitivity/100` and the right channel `throttle − steering·sensitivity/100` (clamped), with left and right outputs diverging proportionally

#### Scenario: Reverse negates both tracks
- **WHEN** gear is R (reverse) and throttle is applied on a skid-steer config
- **THEN** both motor channels operate in reverse relative to their forward direction, with the same differential split applied

#### Scenario: Park locks both tracks
- **WHEN** gear is P (park) on a skid-steer config with throttle and steering applied
- **THEN** both left and right channels are forced to zero (no track creep)

#### Scenario: Brake blending scales both tracks
- **WHEN** `brake_pedal` is pressed beyond the 20% deadband on a skid-steer config
- **THEN** both channels scale down identically and proportionally, reaching zero at full brake

#### Scenario: Right track on the second driver
- **WHEN** `right_motor.hardware` is `DRIVER_B` (or an `S*` ESC) on a skid-steer config
- **THEN** the right channel drives that second output using the same polarity/duty/pulse conventions as the drive motor, and the aux-motor work-machine channel is not initialized (with a boot `WARN` when `aux_motor` is configured)

#### Scenario: Missing track motor warned
- **WHEN** a skid-steer config provides `left_motor` but not `right_motor` (or vice versa)
- **THEN** the firmware logs a `WARN` at boot and the missing channel stays unconfigured while the present one still drives
