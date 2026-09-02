## Context

The RC vehicle firmware combines remote control parsing, physical ESC motor control, lighting automation, and 32-voice I2S audio synthesis. Currently, the drivetrain physics and engine simulation are split across `VehicleController` (which computes motor speed ramping and triggers sound events) and `RcEngineSound` (which calculates an isolated `virtualSpeed`, RPM dynamics, and transmission slip).

This design refactors the architecture into distinct, single-responsibility components:
- `EngineSim`: 50 Hz physics, state machine, RPM dynamics, transmission, and ESC motor speed ramping.
- `SoundSynth`: 22.05 kHz DSP audio synthesis, 32-voice mixing, Hermite spline pitch-shifting, and diesel knock generation.
- `VehicleController`: Hardware I/O dispatcher.

## Goals / Non-Goals

**Goals:**
- Unify powertrain physics and ESC speed calculation into `EngineSim` as the single source of truth.
- Decouple 50 Hz physics from the 22.05 kHz real-time audio synthesis task.
- Simplify `VehicleController` into an I/O dispatcher.
- Maintain full parity with existing vehicle profiles and sound sets.
- Provide clean unit testing of physics and audio rendering in host test harnesses (`host_vc`, `host_dsp`).

**Non-Goals:**
- Changing audio PCM file formats or schema validation.
- Modifying board pin definitions or RadioKit widget UI protocols.
- Adding new vehicle sound samples or altering physical hardware pin mapping.

## Decisions

### 1. Separate `EngineSim` and `SoundSynth` into distinct files
- **Rationale**: Isolates physics (math/state machine) from DSP audio rendering (buffers/Hermite splines). Prevents a 1,200+ line monolithic God-class and allows headless unit testing of physics without audio mocks.
- **Alternatives Considered**: Keeping everything in one `RcEngineSound.cpp` was rejected because it entangles 50 Hz control loops with 22 kHz audio tasks.

### 2. State Snapshot Synchronization
- **Rationale**: `SoundSynth::syncState(const EngineSim& sim)` copies scalar state variables (`rpm`, `pitchFactor`, `state`, `isJakeBrakeActive`) during the 50 Hz control tick.
- **Alternatives Considered**: Direct pointer sharing or callbacks into `SoundSynth` were rejected to avoid race conditions with the FreeRTOS `audioTask` on Core 1.

### 3. Move ESC Motor Ramping (`computeRampedMotorSpeed`) into `EngineSim`
- **Rationale**: Physical ESC motor speed ramping and audio RPM are directly coupled through vehicle inertia, gear ratios, and torque converter slip. Calculating them in one place guarantees 1:1 acoustic and kinematic synchronization.

## Risks / Trade-offs

- **[Risk] Thread safety between 50 Hz `EngineSim` update and 22 kHz `SoundSynth` render** → `SoundSynth::syncState` updates state atomically on Core 0; `renderBlock` on Core 1 reads cached voice states without dynamic allocations or mutex locks.
- **[Risk] Regression in crawler mode or direct drive responsiveness** → `EngineSim` checks `hasEngine` and `inertia == 0` to bypass ramping immediately for direct throttle control.
- **[Risk] Breakage in host test harnesses** → Host harnesses `test/host_vc` and `test/host_dsp` will be updated to test `EngineSim` and `SoundSynth` directly.
