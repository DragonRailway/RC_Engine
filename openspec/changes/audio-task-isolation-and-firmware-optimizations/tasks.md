## 1. Audio Task Isolation & DSP Invariants

- [x] 1.1 Remove blocking `Serial.printf` logging from `audioTask` in `lib/SoundEngine/src/AudioOutput.h` and expose atomic block count.
- [x] 1.2 Add precomputed reciprocal invariants (`invRpmRange`, `invMaxRpm`) in `lib/SoundEngine/src/RcEngineSound.h` and `RcEngineSound.cpp`.
- [x] 1.3 Update `RcEngineSound::update()` to use precomputed reciprocal multipliers for pitch and throttle scaling.

## 2. Decimated 5 Hz Battery ADC & Filtering

- [x] 2.1 Decimate `HardwareInit::readBatteryVoltage()` sampling in `common/VehicleController.cpp` to 5 Hz (every 10 ticks / 200 ms).
- [x] 2.2 Apply exponential moving average (EMA $\alpha=0.1$) low-pass filter to battery voltage in `VehicleController`.

## 3. Fast-Math Compiler Flags & Build Tuning

- [x] 3.1 Add `-ffast-math -fno-math-errno -fno-trapping-math` to `build_flags` in `platformio.ini`.

## 4. Verification & Validation

- [x] 4.1 Verify host test suites (`host_dsp_harness` and `host_vc_harness`).
- [x] 4.2 Build firmware for all targets (`pio run`).
- [x] 4.3 Flash `MIKRO_V2` board and verify clean audio and low battery ADC jitter over serial.
