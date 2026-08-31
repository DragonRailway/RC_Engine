## MODIFIED Requirements

### Requirement: Dynamic Throttle-Dependent Engine Volume
The engine idle and rev sound volumes SHALL scale dynamically with throttle according to `map(currentThrottleFaded, 0, 100, idleVol, fullThrottleVol)`, where `currentThrottleFaded` decays smoothly with an acoustic slew rate upon throttle release matching the virtual flywheel inertia.

#### Scenario: Throttle release volume decay
- **WHEN** user releases the throttle slider from 100% to 0%
- **THEN** engine volume SHALL NOT drop instantaneously, but decay gradually step-by-step following flywheel RPM deceleration down to base idle volume.

#### Scenario: Full throttle volume expansion
- **WHEN** throttle increases from 0% to 100%
- **THEN** engine sound volume expands from base idle volume up to configured `fullThrottle` percentage.

## ADDED Requirements

### Requirement: Phase-Locked Dual-Voice Cylinder Playback
The sound engine SHALL maintain phase synchronization between `voices[IDLE]` and `voices[REV]` by advancing their playhead positions in lockstep and resetting both pointers synchronously upon completion of the master ignition cycle loop.

#### Scenario: Simultaneous Idle and Rev cross-fade playback
- **WHEN** the engine RPM is in the cross-fade region between `revSwitchPoint` and `idleEndPoint`
- **THEN** both IDLE and REV voices advance synchronously with identical phase velocity and wrap at cylinder cycle loop boundaries without phase drift or acoustic phase cancellation.

### Requirement: Cycle-Quantized Jake Brake Transition
The sound engine SHALL quantize the deactivation of jake brake sound and restoration of un-muted engine rev audio to the completion of the current jake brake acoustic loop period, preventing mid-sample audio cuts.

#### Scenario: Jake brake deactivation at loop boundary
- **WHEN** throttle is reapplied or RPM falls below jake brake threshold during active jake braking
- **THEN** jake braking SHALL continue playing until the end of its current audio sample loop before cleanly un-muting the primary engine voices.
