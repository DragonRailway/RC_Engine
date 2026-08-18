## Why

Audio bugs in RC_brain are silent and hard to catch. The most recent crash — an FPU (coprocessor) exception in the audio timer ISR — only surfaced as a runtime panic, and the existing `scripts/smoke_test.py` exercises control widgets but never verifies audio output. The DSP in `RcEngineSound` is deterministic math that nothing tests off-device, while the realtime pipeline (task/ISR pacing, spinlocked voice state, I2S writes) has timing hazards nothing measures. Automatic verification layers catch both classes of bug without listening by ear.

## What Changes

- **A — Sound data validation**: New `scripts/validate_sounds.py` checks every sound JSON off-device: `sampleRate` consistency (22,050 Hz), `sampleCount > 0`, loop-point bounds (`0 ≤ begin < end ≤ count`), and per-slot stats (RMS, peak, DC offset, silence, clipping) with a nonzero exit code on failure.
- **B — Panic watch in smoke test**: `scripts/smoke_test.py` asserts that no panic pattern (`Guru Meditation`, coprocessor/FPU exception, `abort()`) appears in serial during boot **or** the full widget exercise, catching the FPU-ISR bug class as a regression.
- **C — Host DSP unit harness**: Native (x86) build of `RcEngineSound` with minimal Arduino stubs; `scripts/host_dsp_test.py` drives deterministic scripts (start → rev ramps → horn → gear shifts) and asserts on generated sample streams: pitch via zero-crossing rate, loop-region correctness, one-shot voice deactivation, knock cadence via FFT, mixing weights, and no NaN/int8 overflow.
- **D — On-device audio instrumentation**: `-DAUDIO_DEBUG` firmware mode exposing per-buffer stats (peak/RMS/clip-count/NaN), voice activity/position, and audioTask timing (notify latency, I2S write blocking) as structured serial lines; `scripts/audio_capture.py` reassembles the stream into a WAV and runs feature assertions — glitch detection (`|Δsample|` spikes), RMS envelope vs. the driving widget script, zero-crossing rate → reconstructed RPM, and FFT signatures. A serial-triggered self-test mode plays known signals (sine, impulse, sweep, silence) to isolate pipeline health from engine logic.
- **E — Golden metric regression**: Record a per-phase metric profile (RMS/ZCR) from Layer D, store it, and diff against re-runs with tolerance — regression detection without sample-exact (jitter-immune) comparisons.
- **Non-breaking**: All instrumentation sits behind build flags or separate scripts; the production audio path and default firmware behavior are unchanged. Smoke-test additions are additive.

## Capabilities

### New Capabilities
- `audio-debug-tooling`: Automatic verification of audio generation — off-device sound-data validation, panic detection in the E2E smoke test, host-side DSP unit tests, on-device audio instrumentation with waveform capture and feature analysis, and golden metric regression.

### Modified Capabilities
- *(none — all changes are additive; existing capability requirements are unchanged)*

## Impact

- `scripts/validate_sounds.py` (new), `scripts/audio_capture.py` (new), `scripts/host_dsp_test.py` (new), Arduino-stub header for host builds (new).
- `scripts/smoke_test.py` — extended with panic-watch assertions.
- `lib/SoundEngine/src/AudioOutput.h` — `AUDIO_DEBUG` counters/stream and self-test command hook; `RcEngineSound.h` — voice-activity debug accessor (both compile-time gated, no effect on production builds).
- `platformio.ini` — optional native test env for the host DSP harness (does not affect device envs).
- `data/sounds/**` — validated by the new script (no data changes required unless validation fails).
- CI/dev workflow — new scripts runnable from the repo without hardware (A, C) and with hardware (B, D, E).
