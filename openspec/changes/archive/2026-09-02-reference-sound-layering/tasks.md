## 1. SoundSynth Layered Playback

- [x] 1.1 Update `SoundSynth::syncState` to layer IDLE and REV concurrently with dynamic `engineIdle`/`engineRev` to `fullThrottle` volume scaling
- [x] 1.2 Implement cycle-quantized Jake brake deactivation on sample loop wrap in `SoundSynth::renderBlock`
- [x] 1.3 Verify `SoundSynth` phase alignment and soft-knee saturation limiter

## 2. Validation and Hardware Verification

- [x] 2.1 Run host VC and host DSP test suites to verify deceleration envelope behavior
- [x] 2.2 Verify audio waveform with `compare_reference_engine.py` across full acceleration and coastdown cycles
- [x] 2.3 Build and flash firmware to MIKRO_V2 board
- [x] 2.4 Execute `verify_mikro_e2e.py` on hardware to confirm smooth deceleration audio on the real speaker
