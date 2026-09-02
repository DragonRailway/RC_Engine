# sound-synth-engine Specification

## Purpose
Real-time 32-voice audio synthesizer, Hermite cubic spline pitch-shifting, diesel knock generator, and soft-knee limiter driven by `EngineSim` state.

## ADDED Requirements

### Requirement: Engine simulation synchronization
The `SoundSynth` class SHALL provide a `syncState(const EngineSim& sim)` method that reads the current state snapshot (`state`, `rpm`, `pitchFactor`, `jakeBrakeActive`, `wastegateTriggered`, `gear`, `reverseActive`) from `EngineSim` without performing internal drivetrain physics calculations.

#### Scenario: RPM pitch synchronization
- **WHEN** `syncState` is called with an updated `EngineSim`
- **THEN** `SoundSynth` updates engine voice pitch factors, idle/rev volume envelopes, and active engine sound flags accordingly

#### Scenario: Automated sound effect triggering
- **WHEN** `EngineSim` flags jake brake or wastegate active
- **THEN** `SoundSynth` activates the corresponding sound voice slot (`JAKE_BRAKE`, `WASTEGATE`)

### Requirement: 32-voice mixing and Hermite cubic spline interpolation
The `SoundSynth` class SHALL render 16-bit stereo audio blocks (`renderBlock`) using 4-point Hermite cubic spline fractional-step interpolation for pitch-shifted voices and rational soft-knee asymptotic saturation limiting to prevent clipping.

#### Scenario: Multi-voice audio rendering
- **WHEN** engine is running and auxiliary effects (horn, turbo, indicator) are active
- **THEN** `renderBlock` sums active voice samples, applies master/voice volumes, soft-knee limits the mix, and writes interleaved stereo 16-bit PCM frames

### Requirement: Dedicated audio thread execution safety
Audio block rendering in `SoundSynth::renderBlock` SHALL execute inside the FreeRTOS `audioTask` (Core 1) without dynamic heap allocations, blocking mutexes, or filesystem access.

#### Scenario: High-priority DMA audio rendering
- **WHEN** `audioTask` invokes `renderBlock` for 64 audio frames
- **THEN** rendering completes within real-time budget (<1.5 ms) without frame drops or audio buffer underruns
