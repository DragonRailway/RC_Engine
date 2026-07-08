# Parity Gap Closures

## Summary

Close the remaining feature parity gaps between SoundEngine and the reference Rc_Engine_Sound_ESP32 v9.15.0. Focuses on audio quality improvements and missing effect behaviors that affect everyday usability.

## Problem

After the engine simulation parity change, the core engine simulation is complete. However, several everyday sound behaviors are missing or degraded:

1. **Horn/siren/reversing sounds cut off prematurely** — The reference supports configurable loop points so long horns (train horns, fire truck sirens) can sustain. Our implementation plays once and stops.
2. **DAC offset fade missing** — The reference slowly ramps the DAC offset from 0→128 on startup to prevent audio pops. Our I2S output may have similar issues.
3. **RPM-dependent knock volume missing** — The reference scales knock volume with RPM (louder at high RPM, quieter at idle). Our knock volume is fixed, sounding too loud at idle.
4. **Manual transmission shifting sound** — The reference plays a shifting sound when the manual gearbox changes. We have the sound slot but no trigger logic.
5. **Voice mixing weights** — The reference uses weighted mixing (engine at 8/10, effects at 2/10) on separate DAC channels. We mix everything equally into one I2S channel.
6. **Crawler mode** — The reference disables virtual inertia when master volume is low, for precision crawling competitions.

## What We're Building

### Audio Quality (2 features)

1. **Horn/siren/reversing loop points** — Add configurable `loopBegin` and `loopEnd` sample indices to effect voices. When the trigger is still active, the voice jumps back to `loopBegin` instead of stopping. Affects horn, siren, reversing, and sound1.

2. **DAC offset fade (pop prevention)** — Add a startup fade for the I2S output offset, ramping from 0→128 over ~12ms to prevent audio pops on power-up.

### Engine Behavior (3 features)

3. **RPM-dependent knock volume** — Scale knock volume with RPM: quiet at idle, louder at high RPM. Uses a configurable `minKnockVolumePercentage` and `knockStartRpm` threshold.

4. **Manual transmission shifting trigger** — When `transmissionType == MANUAL` and `selectedGear` changes, automatically trigger the shifting sound. Also support a dedicated `triggerShifting()` API.

5. **Voice mixing weights** — Add configurable volume multipliers per voice group (engine vs effects) to prevent clipping when many effects play simultaneously.

### Crawler Mode (1 feature)

6. **Crawler mode** — When master volume is below a configurable threshold, disable virtual inertia (instant RPM response). For precision crawling competitions.

## Why

- These are the remaining gaps between "good" and "reference-quality" audio
- Loop points are the highest-impact fix — long horns sound broken without them
- RPM-dependent knock is the second most noticeable gap — knock too loud at idle
- All features are low-to-medium effort and don't require architectural changes
- Crawler mode is a niche but easy addition

## Scope

**In scope:**
- Modifying `RcEngineSound.h` and `RcEngineSound.cpp` for loop points, knock volume scaling, shifting trigger, mixing weights, crawler mode
- Modifying `AudioOutput.h` for offset fade
- Modifying `VehicleProfile.h` for new config parameters
- Updating vehicle JSON configs with new parameters

**Out of scope:**
- Tire squeal (requires steering input integration)
- Hydraulic/track/bucket sounds (separate vehicle types)
- Out-of-fuel message (battery protection subsystem)
- Web configuration interface
- Dashboard LCD support
- ESP-NOW wireless trailer
