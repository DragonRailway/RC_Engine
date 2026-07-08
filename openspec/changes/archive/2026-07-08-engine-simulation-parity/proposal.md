# Engine Simulation Parity

## Summary

Bring the SoundEngine to feature parity with the reference implementation (Rc_Engine_Sound_ESP32 v9.15.0) for core engine simulation and sound features. This addresses the 7 most impactful gaps identified during the feature parity analysis.

## Problem

The SoundEngine currently produces static, unchanging engine sounds that don't respond realistically to RPM changes. The reference implementation uses variable-rate sample playback (pitch shifting) to create the illusion of an engine revving, along with several other simulation features that make the audio convincing.

Without these features, the SoundEngine sounds like "sound effects playing at constant pitch" rather than "an engine simulation."

## What We're Building

### Tier 1 — Core Engine Simulation (5 features)

1. **Variable-rate sample playback (pitch shifting)** — Engine sounds change pitch with RPM using fractional step interpolation. The single most important feature.

2. **Diesel knock cylinder-adaptive volume** — Knock pulses vary in volume based on engine type (V8, R6, V2 patterns). Trigger-based, fixed pitch, with per-cylinder volume suppression for non-primary pulses.

3. **Jake brake engine slowdown** — Throttle-based auto-trigger. When active, jake brake sound plays instead of engine, and RPM is actively decelerated.

4. **PARKING_BRAKE state** — Full shutdown sequence: RUNNING → STOPPING (fade + pitch drop) → PARKING_BRAKE (air brake sound) → OFF.

5. **Automatic transmission simulation** — Optional per-vehicle torque converter simulation with configurable gear count (3/4/6) and per-gear ramp times.

### Tier 2 — Supporting Features (4 features)

6. **Supercharger start point** — Configurable RPM threshold below which supercharger is silent.

7. **Uncoupling as separate sound** — Separate sound file and trigger for uncoupling vs coupling.

8. **Sound1 generic channel** — Extra programmable sound slot for creative builds (bells, melodies, doors, etc.).

9. **ESC ramp time per gear** — Per-gear acceleration response times, included with automatic transmission.

## Why

- The reference implementation has been proven on ESP32 (weaker than S3) for years
- These 9 features cover the gap between "toy sound effects" and "believable engine simulation"
- The S3 has more than enough CPU budget (~2.6% for pitch shifting with 8 voices)
- Tier 3 features (hydraulics, tracked mode, tire squeal) are deferred as they serve niche vehicle types

## Scope

**In scope:**
- Modifying `RcEngineSound.h`, `RcEngineSound.cpp`, `AudioOutput.h`
- Modifying `VehicleProfile.h` for new config parameters
- Updating vehicle JSON config schemas
- Adding new sound file slots (uncoupling, sound1)

**Out of scope:**
- Tier 3 features (tire squeal, hydraulic/excavator, tracked mode, crawler mode, out-of-fuel)
- Hardware changes
- RC input handling changes (throttle-based auto-jake is implemented in the sound engine, not in input processing)
