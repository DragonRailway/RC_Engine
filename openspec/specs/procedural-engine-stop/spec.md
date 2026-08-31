# Procedural Engine Stop

## Purpose
Defines realistic procedural engine shutdown audio synthesis, including inertial flywheel pitch coast-down, smooth volume decay, cylinder knock cadence stretching, turbo whistle cooldown, vehicle-type timing defaults, and parking brake hand-off.

## Requirements

### Requirement: Procedural engine flywheel spin-down
When an engine stop is requested (`stopEngine()`), `RcEngineSound` SHALL transition to `STOPPING` state and procedurally decay its pitch factor from 1.0 down to a minimum cutoff pitch of `0.18f` over the configured stop duration.

#### Scenario: Pitch wind-down during engine stop
- **WHEN** engine is in `RUNNING` state at idle and `stopEngine()` is invoked
- **THEN** `RcEngineSound` enters `STOPPING` state and `pitchFactor` continuously decreases from 1.0 towards 0.18 over the duration of the stop

### Requirement: Smooth volume envelope decay
During `STOPPING` state, the sound engine SHALL attenuate pitch-shifted engine voices using a smooth linear or cosine volume decay envelope ($1.0 \to 0.0$) across the stop duration, without integer reciprocal stepping cliffs.

#### Scenario: Volume attenuation during stopping
- **WHEN** the engine is in `STOPPING` state at 50% elapsed stop duration
- **THEN** engine voices are rendered at approximately 50% volume with smooth continuous amplitude decay

### Requirement: Cylinder knock cadence stretching
During `STOPPING` state, `voices[KNOCK]` SHALL remain active and advance with the slowed idle loop playback position, naturally stretching the time between knock pulses while scaling knock volume with the engine decay envelope.

#### Scenario: Knock pulse cadence during spin-down
- **WHEN** the engine enters `STOPPING` with knock enabled
- **THEN** knock pulses continue firing with increasing time intervals between strokes until the stop duration completes

### Requirement: Turbo whistle cooldown
During `STOPPING` state, turbocharger whistle volume SHALL fade out over the first 50% of the stop duration.

#### Scenario: Turbo whistle decay
- **WHEN** the engine enters `STOPPING` with turbo active
- **THEN** `voices[TURBO].volume` decays to 0 within the first half of the stop duration

### Requirement: Vehicle type-driven stop duration and config override
The stop duration SHALL default based on vehicle type (1400ms for Truck, 2800ms for Locomotive, 1800ms for Excavator). If `"stop_duration"` is specified under `"engine"` in `vehicle.json`, `ConfigParser` SHALL use the specified millisecond value.

#### Scenario: Default truck stop duration
- **WHEN** a vehicle config with `"type": "truck"` and no explicit `"stop_duration"` is loaded
- **THEN** `cfg.engine.stopDuration` is initialized to 1400ms

#### Scenario: Explicit stop duration override
- **WHEN** a vehicle config declares `"stop_duration": 2500` under `"engine"`
- **THEN** `cfg.engine.stopDuration` is set to 2500ms

### Requirement: Parking brake hand-off
When the stop duration elapses or pitch reaches the minimum cutoff threshold, `RcEngineSound` SHALL immediately transition to `PARKING_BRAKE` state (if `PARKING_BRAKE` sound is available) or `OFF` with zero dead silence delay.

#### Scenario: Immediate air release transition
- **WHEN** the stop duration completes and parking brake sound is present
- **THEN** `state` transitions immediately to `PARKING_BRAKE` and `voices[PARKING_BRAKE].active` is set to true
