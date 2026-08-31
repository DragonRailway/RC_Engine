## Context

The ESP32-S3 runs dual Xtensa LX7 cores at 240 MHz. Real-time audio rendering occurs at 22,050 Hz in 64-sample frames (every ~2.9 ms) in `audioTask` (Priority 3, Core 1), while vehicle physics, steering auto-centering, and lighting run at 50 Hz in `controlTask` (Priority 2, Core 1).

## Goals / Non-Goals

**Goals:**
- Eliminate all potential blocking I/O (UART/USB CDC `Serial.printf`) from the Priority 3 `audioTask`.
- Decimate ADC battery voltage sampling to 5 Hz (every 200 ms) with an exponential moving average low-pass filter, saving 90% of ADC sampling time and smoothing out motor ripple.
- Precalculate invariant division terms (`invRpmRange = 1.0f / (maxRpm - idleEndPoint)`, etc.) in `RcEngineSound::setConfig()`, converting runtime DSP divisions to single-cycle multiplications.
- Enable fast-math FPU compiler optimizations across all board targets.

**Non-Goals:**
- Modifying RadioKit network packet structures or LittleFS partition layout.

## Decisions

### 1. Zero-I/O Audio Task
- **Decision**: `audioTask` in `AudioOutput.h` will perform purely mathematical DSP rendering and I2S DMA buffer writes. Diagnostics block counters will be stored in `static volatile uint32_t s_blockCount` for asynchronous readout by diagnostic monitors or `UiLogger`.
- **Rationale**: Any blocking call in Priority 3 preempts all other tasks and can stall audio output if the serial buffer is full.

### 2. Decimated 5 Hz Battery Voltage Sampling with EMA Filter
- **Decision**: In `VehicleController::update()`, execute `HardwareInit::readBatteryVoltage()` once every 10 ticks (200 ms). Update `s_filteredBatV = s_filteredBatV * 0.9f + rawV * 0.1f`.
- **Rationale**: Battery chemistry does not change at 50 Hz. Sampling at 5 Hz saves ~35 µs of CPU blocking every 20 ms and filters out PWM switching noise caused by high motor loads.

### 3. Reciprocal DSP Invariants
- **Decision**: In `RcEngineSound`, add member fields `float invRpmRange`, `float invMaxRpm`, `float invJakeDecelRate`. Compute them once during `setConfig()`.
- **Rationale**: Single-precision floating-point division takes 14–16 CPU cycles on Xtensa LX7, whereas multiplication takes 1 cycle. In tight pitch scaling loops, precomputed reciprocals reduce CPU cycles significantly.

### 4. Compiler Fast-Math Flags
- **Decision**: Add `-ffast-math -fno-math-errno -fno-trapping-math` to `platformio.ini`.
- **Rationale**: Allows the GCC compiler to reorder floating-point operations and generate single-cycle fused multiply-accumulate (`madd.s`) instructions for Hermite cubic interpolation and soft-knee saturation.

## Risks / Trade-offs

- [Risk] Fast-math might change NaN or denormal handling.
  - *Mitigation*: Audio DSP operates strictly within normalized floating-point ranges `[-1.0f, +1.0f]` and samples are bounded by `constrain()` and polynomial saturation. Host DSP test suites verify exact numerical stability.
