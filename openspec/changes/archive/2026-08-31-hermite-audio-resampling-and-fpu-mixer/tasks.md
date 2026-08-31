## 1. Hermite 4-Point Spline Resampling Implementation

- [x] 1.1 Implement `readInterpolatedHermite4p` in `lib/SoundEngine/src/RcEngineSound.h` with loop and one-shot boundary handling.
- [x] 1.2 Replace linear interpolation in `renderBlock()` in `lib/SoundEngine/src/RcEngineSound.cpp` with 4-point Hermite cubic resampling.

## 2. Float Vectorized Voice Gain Stage

- [x] 2.1 Refactor `RcEngineSound::renderBlock()` in `lib/SoundEngine/src/RcEngineSound.cpp` to precompute float voice gains before the frame loop.
- [x] 2.2 Vectorize inner sample accumulation using hardware FPU operations and float-to-int16 clamping.
- [x] 2.3 Streamline `AudioOutput::audioTask` volume ramp scaling to float multiplication.

## 3. Verification & Regression Testing

- [x] 3.1 Update host DSP harness (`test/host_dsp/host_dsp_driver.cpp`) with Hermite interpolation mathematical precision tests and ZCR checks.
- [x] 3.2 Run host test harnesses (`host_dsp_harness` and `host_vc_harness`) to verify zero regressions.
- [x] 3.3 Validate configs and verify PlatformIO build across all environments (`pio run`).
