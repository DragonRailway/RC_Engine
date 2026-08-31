## Why

When driving RC vehicles, releasing the throttle slider or transitioning between rev and idle causes the rev sound to cut off abruptly or snap back harshly. Aligning the sound engine with reference behavior (Rc_Engine_Sound_ESP32) resolves this through phase-locked sample playback, slew-rate volume decay, and cycle-quantized jake braking.

## What Changes

- **Phase-Locked IDLE & REV Loops**: Synchronize `voices[IDLE]` and `voices[REV]` fractional playhead pointers so cylinder ignition cycles remain phase-locked in lockstep across cross-fading.
- **Slew-Rate Throttle Volume Decay**: Implement smooth acoustic volume inertia (`currentThrottleFaded` decay) so engine volume matches virtual flywheel deceleration rather than snapping immediately on slider release.
- **Cycle-Quantized Jake Brake & Smooth Mute**: Quantize jake brake exit to the end of complete acoustic loop cycles and blend/duck the base rev rumble instead of hard-muting to zero.

## Capabilities

### New Capabilities
<!-- None -->

### Modified Capabilities
- `audio-mixing-pipeline`: Adds phase-locked dual-voice playback synchronization, slew-rate throttle volume fading, and cycle-quantized engine compression brake transitions.

## Impact

- `lib/SoundEngine/src/RcEngineSound.h` & `RcEngineSound.cpp`: Audio mixer loop synchronization, throttle volume slew tracking, and jake brake state management.
- Test suites: Updated DSP and vehicle controller test harnesses verifying smooth acoustic transitions.
