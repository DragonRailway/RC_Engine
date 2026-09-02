## MODIFIED Requirements

### Requirement: Engine simulation synchronization
The `SoundSynth` class SHALL provide a `syncState(const EngineSim& sim)` method that reads the current state snapshot (`state`, `rpm`, `pitchFactor`, `jakeBrakeActive`, `wastegateTriggered`, `gear`, `reverseActive`) from `EngineSim` without performing internal drivetrain physics calculations:
1. `IDLE` and `REV` sound voices SHALL play concurrently when in running states (or direct mode).
2. `IDLE` volume SHALL scale dynamically: `map(throttlePct, 0, 100, engineIdleVolume, fullThrottleVolume)`.
3. `REV` volume SHALL scale dynamically: `map(throttlePct, 0, 100, engineRevVolume, fullThrottleVolume)`.
4. When `jakeBrakeActive` is true, engine sound voices SHALL be muted while the Jake brake sample plays.

#### Scenario: RPM pitch synchronization
- **WHEN** `syncState` is called with an updated `EngineSim`
- **THEN** `SoundSynth` updates engine voice pitch factors, layered idle/rev volume envelopes, and active sound flags accordingly

#### Scenario: Concurrent idle and rev layering during deceleration
- **WHEN** vehicle throttle drops to 0% while engine RPM is spinning down from high RPM
- **THEN** both IDLE and REV voices continue playing concurrently at their baseline idle volume levels tracking flywheel pitch down to idle

#### Scenario: Automated sound effect triggering
- **WHEN** `EngineSim` flags jake brake or wastegate active
- **THEN** `SoundSynth` activates the corresponding sound voice slot (`JAKE_BRAKE`, `WASTEGATE`)

## ADDED Requirements

### Requirement: Cycle-quantized jake brake deactivation
The `SoundSynth` class SHALL only deactivate the `JAKE_BRAKE` sound voice and unmute base engine voices at the conclusion of a sample loop wraparound (`wrapped == true`) when `jakeBrakeRequest` is false.

#### Scenario: Jake brake loop completion
- **WHEN** Jake brake request transitions to false while Jake brake audio is playing
- **THEN** Jake brake voice continues playing until the end of its current audio loop, then un-mutes engine voices without audio discontinuities
