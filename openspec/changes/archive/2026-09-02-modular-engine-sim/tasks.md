## 1. EngineSim Component Implementation

- [x] 1.1 Create `lib/SoundEngine/src/EngineSim.h` defining `EngineSim` class, state machine, control input struct, and physics configuration
- [x] 1.2 Implement `lib/SoundEngine/src/EngineSim.cpp` with RPM slew rates, automatic/manual transmission, torque converter slip, and ESC motor speed ramping (`computeRampedMotorSpeed`)
- [x] 1.3 Add isolated unit test in `test/host_vc` validating `EngineSim` RPM response, gear changes, dynamic braking, and direct mode bypass

## 2. SoundSynth Refactoring

- [x] 2.1 Create `lib/SoundEngine/src/SoundSynth.h` refactoring `RcEngineSound` into `SoundSynth`, isolating 32-voice mixing and DSP functions
- [x] 2.2 Implement `lib/SoundEngine/src/SoundSynth.cpp` with `syncState(const EngineSim&)` and Hermite cubic spline block rendering
- [x] 2.3 Update `lib/SoundEngine/src/AudioOutput.h` to drive `SoundSynth` from the FreeRTOS audio task
- [x] 2.4 Update `test/host_dsp` to verify `SoundSynth` rendering and pitch scaling

## 3. VehicleController & Main Integration

- [x] 3.1 Update `VehicleProfile.h` and `Config.h` to bind configuration to `EngineSim` and `SoundSynth`
- [x] 3.2 Refactor `common/VehicleController.h` and `VehicleController.cpp` to delegate all drivetrain physics to `EngineSim` and sync with `SoundSynth`
- [x] 3.3 Update `src/main.cpp` for clean initialization and teardown of `EngineSim`, `SoundSynth`, and `AudioOutput`

## 4. Verification & Regression Testing

- [x] 4.1 Run host unit test harnesses (`pio test -e TRACKLINK_V3` / host drivers)
- [x] 4.2 Validate config loading and bundle builds with `scripts/build_fs.py --board MIKRO_V2 --vehicle ScaniaV8 --dry-run`
- [x] 4.3 Build target firmware binaries (`pio run -e MIKRO_V2` and `pio run -e TRACKLINK_V3`)
