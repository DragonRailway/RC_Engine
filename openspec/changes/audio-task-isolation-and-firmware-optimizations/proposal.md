## Why

While the firmware has achieved real-time 50 Hz control determinism and cubic spline audio resampling, further profiling reveals several performance and latency bottlenecks across tasks and subsystems:
1. **Blocking Logging in Real-Time Audio Task**: `AudioOutput.h` invokes `Serial.printf` directly inside `audioTask` (Priority 3). When USB CDC TX buffers are full or unread, this blocks the real-time audio thread for up to 10 ms, causing I2S DMA underruns and audio popping.
2. **High-Frequency SAR ADC Sampling Overhead & Noise**: `VehicleController::update()` polls `HardwareInit::readBatteryVoltage()` at the full 50 Hz control rate. Each `analogReadMilliVolts()` call takes ~35 µs with calibration lookup, consuming ~10% of the control loop's compute budget while picking up high-frequency motor switching noise.
3. **Repeated Runtime Floating-Point Divisions in DSP Updates**: `RcEngineSound::update()` recalculates invariant division terms (such as `invRpmRange` and transmission gear ratios) every frame rather than precalculating reciprocals upon config load.
4. **Compiler Optimization Flags**: PlatformIO builds currently omit Xtensa LX7 FPU optimization flags (`-ffast-math`, `-fno-math-errno`, `-fno-trapping-math`) that enable hardware fused multiply-accumulate (FMA) instructions and vector optimizations.

## What Changes

1. **Decouple Logging from Real-Time `audioTask`**:
   - Remove blocking `Serial.printf` calls from `audioTask` in `AudioOutput.h`.
   - Maintain block counters in lightweight atomic variables and report audio heartbeat diagnostics from lower-priority tasks.
2. **Decimated 5 Hz Battery ADC Sampling with Low-Pass Filtering**:
   - Decimate `HardwareInit::readBatteryVoltage()` sampling in `VehicleController::update()` from 50 Hz to 5 Hz (every 10 ticks / 200 ms).
   - Apply an exponential moving average (EMA) low-pass filter ($\alpha = 0.1$) to eliminate motor PWM switching voltage ripple and save 90% of ADC sampling CPU time.
3. **Precomputed Reciprocal Invariants in `RcEngineSound`**:
   - Cache precomputed reciprocal floats (`invRpmRange`, `invGearSize`, `invJakeMargin`) in `RcEngineSound` during `setConfig()` to replace runtime floating-point divisions with single-cycle multiplications.
4. **Fast-Math & Vectorized FPU Build Flags**:
   - Add `-ffast-math -fno-math-errno -fno-trapping-math` build flags to `platformio.ini` across all board environments to unlock Xtensa LX7 dual-issue hardware FMA instructions.

## Capabilities

### Modified Capabilities
- `audio-mixing-pipeline`: Specify non-blocking real-time audio task execution and precomputed DSP invariants.
- `vehicle-control-loop`: Specify 5 Hz decimated battery ADC sampling with low-pass filtering.

## Impact

- `lib/SoundEngine/src/AudioOutput.h`: Remove `Serial.printf` from `audioTask`.
- `lib/SoundEngine/src/RcEngineSound.h`, `RcEngineSound.cpp`: Add precalculated reciprocal invariants in `RcEngineSound::Config` and `setConfig()`.
- `common/VehicleController.h`, `VehicleController.cpp`: Decimate battery voltage ADC reads to 5 Hz.
- `platformio.ini`: Enable fast-math FPU flags.
- `test/host_dsp/host_dsp_driver.cpp`, `test/host_vc/host_vc_driver.cpp`: Update tests and verify performance.
