## 1. Deterministic Control Task & Event-Driven Reload (Section 1)

- [ ] 1.1 Create FreeRTOS `controlTask` pinned to Core 1 at priority 2 with strict 50 Hz (20 ms) `vTaskDelayUntil()` loop in `src/main.cpp`.
- [ ] 1.2 Move `VehicleController::update()` and `HardwareInit::update()` from `loop()` to `controlTask`.
- [ ] 1.3 Implement event-driven config reload callback from RadioKit and eliminate periodic `stat()` file checking from `loop()`.

## 2. Warm Soft-Knee Limiter & Continuous Float Inertia (Section 2)

- [ ] 2.1 Implement `saturateSoftKnee()` in `lib/SoundEngine/src/RcEngineSound.h` using FPU-accelerated cubic polynomial formulation.
- [ ] 2.2 Apply `saturateSoftKnee()` at the output stage of `RcEngineSound::renderBlock()` in `lib/SoundEngine/src/RcEngineSound.cpp`.
- [ ] 2.3 Refactor `RcEngineSound::update()` to continuous floating-point exponential RPM inertia and torque converter slip modeling.

## 3. Verification & Validation

- [ ] 3.1 Update host DSP harness (`test/host_dsp/host_dsp_driver.cpp`) with soft-knee saturator and continuous RPM tests.
- [ ] 3.2 Run host test harnesses (`host_dsp_harness` and `host_vc_harness`).
- [ ] 3.3 Build firmware across all PlatformIO targets (`pio run`).
- [ ] 3.4 Flash `MIKRO_V2` board and verify deterministic 50 Hz control and clean audio over serial.
