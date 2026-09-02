## Why

Currently, engine and drivetrain simulation is split across `VehicleController` (which computes motor ESC inertia, speed ramps, and manual sound triggers) and `RcEngineSound` (which calculates an isolated `virtualSpeed`, RPM dynamics, and transmission slip). This split causes duplicate state, synchronization lag between motor response and audio synthesis, and bloats `VehicleController` with non-I/O physics logic.

Refactoring the simulation into a dedicated `EngineSim` class and refactoring audio synthesis into `SoundSynth` unifies the powertrain physics into a single source of truth, decouples 50 Hz physics from 22.05 kHz audio DSP, and simplifies `VehicleController` into a lean hardware I/O dispatcher.

## What Changes

- **New Component `EngineSim` (`lib/SoundEngine/src/EngineSim.h`, `EngineSim.cpp`)**: Consolidates the engine state machine (OFF, STARTING, RUNNING, STOPPING, PARKING_BRAKE), RPM slew rates, automatic/manual transmission with torque converter slip, and ESC motor speed ramping (`computeRampedMotorSpeed`).
- **Refactored Component `SoundSynth` (`lib/SoundEngine/src/SoundSynth.h`, `SoundSynth.cpp`)**: Replaces/refactors `RcEngineSound` to focus purely on 32-voice audio mixing, 4-point Hermite spline interpolation, diesel knock synthesis, and audio block rendering (`renderBlock`), synchronizing state from `EngineSim`.
- **Simplified `VehicleController` (`common/VehicleController.h`, `VehicleController.cpp`)**: Removes duplicate physics variables (`s_currentMotorSpeed`, `computeRampedMotorSpeed`) and acts as a pure I/O router between RadioKit widgets, `EngineSim`, `SoundSynth`, and hardware drivers (`HardwareInit`).
- **Updated Host Test Harnesses (`test/host_vc`, `test/host_dsp`)**: Updates and expands host test suites to verify `EngineSim` physics in isolation and validate `SoundSynth` audio rendering.

## Capabilities

### New Capabilities
- `engine-sim-powertrain`: Dedicated powertrain and engine simulation model computing engine state, RPM, transmission gearing, torque converter slip, and ESC motor speed output.
- `sound-synth-engine`: Real-time audio synthesizer and 32-voice mixer driven by `EngineSim` state.

### Modified Capabilities
- `vehicle-control-loop`: Streamlines the control loop to feed inputs to `EngineSim`, sync `SoundSynth`, and dispatch outputs to motors, servos, and lights without internal motor inertia math.

## Impact

- **Codebase**: `lib/SoundEngine/src/` gains `EngineSim.h/.cpp` and `SoundSynth.h/.cpp`; `RcEngineSound.h/.cpp` is migrated/deprecated in favor of `SoundSynth`; `VehicleController` is significantly simplified.
- **APIs**: `VehicleController` interacts with `EngineSim` for driving and `SoundSynth` for audio events.
- **Dependencies**: None changed (ESP-IDF, Arduino, FreeRTOS).
- **Tests**: Host harnesses `test/host_vc` and `test/host_dsp` updated for the new class interfaces.
