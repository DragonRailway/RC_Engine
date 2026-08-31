## Why

When an engine stop is requested, the current firmware produces an abrupt and unrealistic sound cutoff. The root causes are:
1. `stopPitchFactor` is clamped to `>= 1.0f`, preventing the rotational assembly from winding down below idle frequency.
2. Volume uses reciprocal integer division (`scaled / attenuator`), dropping 50% in the first 80ms and cutting to silence within 300ms.
3. Diesel knock is gated by `state == RUNNING`, cutting off cylinder firing strokes instantly upon stop trigger.
4. A 3.2-second fixed delay occurs before the parking brake air release sound triggers, leaving an awkward dead silence.

Implementing a realistic procedural flywheel spin-down, smooth envelope decay, cadence-stretched knock compression, and type-driven stop timing will deliver authentic acoustic shutdown behavior across all vehicle models without requiring new sample recordings.

## What Changes

- **Procedural Flywheel Coast-Down**: Decay `pitchFactor` from 1.0 down to `minPitch` (0.18) using an inertial deceleration curve.
- **Smooth Volume Envelope**: Replace reciprocal `scaled / attenuator` with a smooth decay envelope ($1.0 \to 0.0$) across the stop duration.
- **Cylinder Compression Cadence Stretching**: Keep `voices[KNOCK]` active and advancing with the decelerating idle sample loop, naturally stretching the firing interval while fading knock volume smoothly.
- **Turbo Whistle Cooldown**: Fade turbo whistle over the first 50% of the stop duration.
- **Type-Driven Smart Defaults & Config Override**: Default stop duration to 1400ms (Truck), 2800ms (Locomotive), 1800ms (Excavator), with optional `"stop_duration"` integer (ms) in `vehicle.json` under `engine`.
- **Immediate Parking Brake Hand-off**: Transition to `PARKING_BRAKE` immediately upon spin-down completion with zero dead delay.

## Capabilities

### New Capabilities
- `procedural-engine-stop`: Procedural flywheel inertia spin-down, smooth volume envelope, cylinder knock cadence stretching, and type-driven stop timing.

### Modified Capabilities
*(None. Existing hardware state transitions and user controls remain unchanged.)*

## Impact

- **`lib/SoundEngine/src/RcEngineSound.h` / `RcEngineSound.cpp`**: STOPPING state machine, flywheel pitch decay, smooth volume envelope, knock continuity, and parking brake hand-off.
- **`common/Config.h`**: Added `stopDuration` parameter in `EngineConfig`.
- **`common/ConfigParser.cpp`**: Parse `engine.stop_duration` (or `engine.stop_duration_ms`) with vehicle type-dependent defaults.
- **`configs/schemas/vehicle_config.schema.json`**: Optional schema definition for `stop_duration`.
