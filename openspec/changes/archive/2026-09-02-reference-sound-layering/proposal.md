## Why

During vehicle deceleration, engine rev sound abruptly stopped or suffered volume collapse because of artificial RPM-crossfading and aggressive muting transitions. Aligning the SoundSynth mixer with the reference layered playback model eliminates abrupt audio drops during coasting and deceleration, ensuring continuous, natural engine audio across all throttle transitions.

## What Changes

- **Reference Layered IDLE/REV Mixing**: Run `IDLE` and `REV` sound voices concurrently during engine running states, dynamically scaling both voices from their baseline idle volumes (`engineIdle` and `engineRev`) up to `fullThrottle` according to throttle load and flywheel dynamics without artificial RPM-cutoffs.
- **Cycle-Quantized Jake Brake Audio Isolation**: Mute IDLE/REV voices during active Jake braking, and cleanly release Jake brake audio upon sample loop wraparound.
- **Smooth Deceleration RPM & Volume Tracking**: Ensure engine sound volume during coastdown smoothly decays with flywheel inertia rather than dropping instantaneously when the gas pedal is released.

## Capabilities

### New Capabilities
None.

### Modified Capabilities
- `sound-synth-engine`: Update engine voice mixing to use concurrent layered playback of IDLE and REV voices with dynamic throttle load scaling and cycle-quantized Jake brake transitions.

## Impact

- `lib/SoundEngine/src/SoundSynth.cpp` & `lib/SoundEngine/src/SoundSynth.h`
- `lib/SoundEngine/src/EngineSim.cpp` & `lib/SoundEngine/src/EngineSim.h`
- `test/` and `scripts/` DSP harnesses.
