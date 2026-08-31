## Why

The sound engine currently resamples audio using 2-point linear interpolation and computes multi-voice sample mixing through repeated per-sample software integer divisions. At higher engine RPMs (pitch shifting above 1.0x), linear interpolation introduces audible high-frequency aliasing and slope distortion, while repeated integer divisions consume unnecessary CPU cycles in the real-time audio task.

Upgrading to 4-point Hermite cubic spline interpolation and precomputing floating-point voice gain coefficients leverages the ESP32-S3 hardware single-precision FPU to significantly improve sound quality while speeding up audio block rendering.

## What Changes

- **4-Point Hermite Cubic Spline Resampling**: Replace linear 2-point interpolation in `readInterpolated` with a 4-point Catmull-Rom/Hermite cubic polynomial spline, providing $C^1$ smooth curvature across fractional playback positions and eliminating high-RPM aliasing.
- **Precomputed Floating-Point Voice Gains**: Precalculate combined voice gains (`voice.volume * groupGain * mixWeight * master`) as float coefficients before the 64-frame rendering loop, replacing 4–5 software integer divisions per sample with single-cycle hardware FPU multiply-accumulate operations.
- **Optimized Volume Ramping**: Streamline fade-in gain scaling in `AudioOutput::audioTask` to float multiplication.

## Capabilities

### Modified Capabilities
- `audio-mixing-pipeline`: Update sound rendering requirements to specify 4-point Hermite cubic spline interpolation and precomputed float voice gain mixing.

## Impact

- `lib/SoundEngine/src/RcEngineSound.h`: Hermite 4-point sample reader with loop boundary handling.
- `lib/SoundEngine/src/RcEngineSound.cpp`: Float-vectorized `renderBlock()` mixer pipeline.
- `test/host_dsp/host_dsp_driver.cpp`: Host DSP tests verifying Hermite interpolation accuracy, frequency response, and zero clipping.
- `test/host_vc/host_vc_driver.cpp`: Full end-to-end regression validation.
