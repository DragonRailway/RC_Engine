## Why

The current firmware executes the vehicle control loop, drivetrain physics, and LED/servo animations inside the main Arduino `loop()`. Because `loop()` also services BLE packets, RPC requests, and periodic filesystem `stat()` calls, the loop execution period $\Delta t$ fluctuates unpredictably between 1 ms and 25 ms. This loop jitter causes variable integration steps in virtual mass inertia, transmission shift ramps, and dynamic steering auto-centering.

In addition, loud simultaneous sound bursts (e.g. engine rev + horn + air brake + turbo at full throttle) currently hard-clamp to $\pm 32767$, producing harsh digital distortion, while the virtual flywheel simulation in `RcEngineSound` relies on integer step truncation, creating quantization notches at low idle RPMs.

## What Changes

1. **Deterministic 50 Hz Periodic FreeRTOS Control Task**:
   - Move `VehicleController::update()` and `HardwareInit::update()` to a dedicated FreeRTOS task `controlTask` pinned to Core 1 at priority 2 using `vTaskDelayUntil()`.
   - Guarantees strict 20.000 ms periodic timing for all drivetrain physics, steering decay, and animation curves.

2. **Event-Driven Config Hot-Reload**:
   - Eliminate the 2-second periodic `stat()` LittleFS file polling from the main loop.
   - Trigger configuration reloads exclusively when RadioKit signals a completed filesystem upload.

3. **Warm Analog Soft-Knee Limiter (Anti-Clipping)**:
   - Implement an FPU-accelerated cubic polynomial soft saturator at the output stage of `RcEngineSound::renderBlock()`.
   - Prevents harsh digital clipping during multi-voice volume peaks while remaining 100% transparent at normal levels.

4. **Continuous Floating-Point Engine Flywheel & Converter Simulation**:
   - Refactor `RcEngineSound::update()` to track RPM and inertia ramps as continuous floating-point differential equations.
   - Eliminates integer division truncation notches and enables sub-RPM precision modeling of engine idle and torque converter slip.

## Capabilities

### Modified Capabilities
- `vehicle-control-loop`: Specify deterministic 50 Hz periodic execution and event-driven configuration reloading.
- `audio-mixing-pipeline`: Specify polynomial soft-knee output limiting and continuous floating-point RPM inertia integration.

## Impact

- `src/main.cpp`: FreeRTOS `controlTask` creation, elimination of periodic `stat()` file polling in `loop()`.
- `lib/SoundEngine/src/RcEngineSound.h`: Continuous float RPM members and inline cubic soft saturator.
- `lib/SoundEngine/src/RcEngineSound.cpp`: Soft-knee limiting in `renderBlock()` and continuous float flywheel integration in `update()`.
- `test/host_dsp/host_dsp_driver.cpp`: Host DSP tests for soft-knee saturation, zero clipping, and continuous RPM ramps.
- `test/host_vc/host_vc_driver.cpp`: Verification of deterministic control integration.
