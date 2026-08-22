## 1. Voice Definition and Gain Staging Alignment

- [x] 1.1 Update `voiceDefs` in `RcEngineSound.cpp` with C++ designated initializers mapping each `SoundID` enum key explicitly to its loop, oneShot, and pitchShift properties.
- [x] 1.2 Implement sub-mix gain multipliers (0.8x engine/horn, 0.2x auxiliary turbo/fan/supercharger, 0.2x group B knock/wastegate/brake/reversing) in `RcEngineSound`.
- [x] 1.3 Update throttle-dependent engine volume curve in `RcEngineSound::update()` to scale smoothly from idle to full throttle.

## 2. 16-Bit Block-Based Audio Rendering

- [x] 2.1 Declare `renderBlock(int16_t* interleavedStereoBuffer, size_t frames)` in `RcEngineSound.h`.
- [x] 2.2 Implement `renderBlock()` in `RcEngineSound.cpp` with single-lock voice snapshotting, 32-bit headroom accumulation, 16-bit PCM output clipping, and Left/Right duplication.
- [x] 2.3 Retain a backwards-compatible `getNextSample()` in `RcEngineSound` for single-sample harnesses.

## 3. AudioOutput Pipeline Integration

- [x] 3.1 Update `AudioOutput::audioTask` in `AudioOutput.h` to call `engine->renderBlock(buffer, BUFFER_SIZE)` directly.
- [x] 3.2 Adjust volume fade ramp in `AudioOutput.h` to scale 16-bit samples cleanly.

## 4. Verification and DSP Validation

- [x] 4.1 Build and run the host DSP verification test harness (`scripts/compare_reference_engine.py` or `scripts/host_dsp_test.py`).
- [x] 4.2 Verify PlatformIO compilation across `MIKRO_V2` and `TRACKLINK_V3` build environments.
