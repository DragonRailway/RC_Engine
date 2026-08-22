## Why

Analysis of the audio output against the reference ESP32 sound engine (`Rc_Engine_Sound_ESP32`) identified critical mixing defects:
1. `voiceDefs` array index desynchronization causing improper pitch shifting, premature cutoff of the fan, infinite looping of the air brake, and pitch-shifted sirens.
2. Missing reference sub-mix gain staging (0.8x engine, 0.2x turbo/fan/supercharger, 0.2x group B effects), causing diesel knock and transient effects to play 5x too loud with severe digital clipping.
3. An 8-bit intermediate sample bottleneck (`uint8_t` +128 DC offset clamp) before upscaling to 16-bit I2S, which introduces 48 dB SNR quantization noise and distortion.
4. Per-sample `memcpy` and critical sections (22,050 times/second) creating unnecessary CPU overhead.

Fixing these defects and upgrading to native 16-bit block-based rendering will deliver clean, authentic audio with high dynamic range and minimal CPU load.

## What Changes

- **Designated Initializer Voice Definitions**: Map `voiceDefs` explicitly using `SoundID` enum indices (`[IDLE] = {...}`, `[JAKE_BRAKE] = {...}`, etc.) to guarantee 100% alignment.
- **Reference-Accurate Gain Staging**: Apply reference attenuation multipliers:
  - Engine voice group (Idle, Rev, Jake Brake, Start): $\times 0.8$
  - Engine auxiliary group (Turbo, Fan, Supercharger): $\times 0.2$
  - Ancillary / Triggered effects group B (Knock, Wastegate, Air Brake, Parking Brake, Shifting, Reversing, Coupling, Uncoupling): $\times 0.2$
  - Horn / Siren: $\times 0.8$
  - Excavator & Additional groups (Hydraulic Flow, Track Rattle, Bucket Rattle, Tire Squeal): $\times 1.0$
- **Dynamic Throttle Engine Volume**: Align throttle-dependent engine volume curve with `map(throttlePercent, 0, 100, idleVol, fullThrottleVol)`.
- **Native 16-Bit Signed Block Rendering**:
  - Implement `renderBlock(int16_t* buffer, size_t frames)` in `RcEngineSound` to render 64 stereo frames in a single call.
  - Mix directly into 32-bit accumulators with digital headroom and clamp to `int16_t` signed PCM ($-32768 \dots 32767$), eliminating the 8-bit quantization bottleneck and DC bias round-trip.
- **Performance Optimization**: Reduce task synchronization from 22,050 critical section pairs/sec down to 344/sec (once per buffer) and only iterate active voices.

## Capabilities

### New Capabilities
- `audio-mixing-pipeline`: Defines requirements for 16-bit block-based multi-voice audio mixing, gain staging factors, voice metadata alignment, and I2S output formatting.

### Modified Capabilities
*(None - no external requirement changes to existing specs)*

## Impact

- `lib/SoundEngine/src/RcEngineSound.h`: Add `renderBlock(int16_t* buffer, size_t frames)`, update `getNextSample()` for compatibility if needed.
- `lib/SoundEngine/src/RcEngineSound.cpp`: Fix `voiceDefs` array, implement reference gain staging, update volume scaling math, implement `renderBlock()`.
- `lib/SoundEngine/src/AudioOutput.h`: Update `audioTask` to invoke `engine->renderBlock()` directly into the I2S DMA buffer.
- `test/host_dsp/`: Update unit tests / harness to verify 16-bit block rendering and reference mixing parity.
