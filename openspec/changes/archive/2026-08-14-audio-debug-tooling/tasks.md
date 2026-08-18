## 1. Layer A — Sound Data Validation

- [x] 1.1 Write `scripts/validate_sounds.py`: walk `data/sounds/`, load every sound JSON, validate `sampleRate` (22050), `sampleCount > 0`, loop-point bounds (`0 ≤ begin < end ≤ count`) for all looped voices, and per-slot stats (RMS, peak, DC offset, all-zero/silence, clipping); exit nonzero on failure with per-file reporting
- [x] 1.2 Run `scripts/validate_sounds.py` against the current `data/sounds/` tree; fix any genuine data issues found (or document expected-fail cases)

## 2. Layer B — Panic Watch in Smoke Test

- [x] 2.1 Add a shared panic-pattern list (`Guru Meditation|Coprocessor exception|abort()|Backtrace`) to `scripts/smoke_test.py` and assert no match during the existing 12 s boot drain
- [x] 2.2 Add a per-phase panic check after each widget phase during the exercise; FAIL + nonzero exit on any match

## 3. Layer C — Host DSP Harness

- [x] 3.1 Create the stub `Arduino.h` (millis, no-op Serial, no-op `portMUX` macros, `random()`) and verify `RcEngineSound.cpp` compiles and links clean on x86 with it
- [x] 3.2 Write `scripts/host_dsp_test.py` (+ small C++ driver) that takes a deterministic script (start → rev ramp → horn → gear → stop) and emits raw int8 PCM + voice activity logs
- [x] 3.3 Implement assertions on generated streams: pitch via zero-crossing rate vs. commanded RPM, loop-region respect, one-shot deactivation, knock cadence via FFT, no NaN / no int8 overflow
- [x] 3.4 Prove the harness catches regressions: introduce a scratch break (e.g. bad loop bound) in a copy and confirm the corresponding test fails

## 4. Layer D — On-Device Instrumentation & Capture

- [x] 4.1 Add `AUDIO_DEBUG` per-buffer counters in `AudioOutput::audioTask` (peak, RMS, clip count, DC offset) plus task-loop and `i2s_channel_write` timing via `esp_timer_get_time()`; emit structured `[AUDIO_STATS]` lines at a bounded rate
- [x] 4.2 Add `#ifdef AUDIO_DEBUG` `isfinite()` checks on mix accumulators in `getNextSample` and a `debugVoiceSnapshot()` accessor (active/position/count/loop bounds per voice); emit `[AUDIO_VOICE]` lines once per second
- [x] 4.3 Implement the serial-triggered self-test mode (sine 440 Hz, impulse, sweep, silence) that replaces `engine->getNextSample()` in the audio task for a bounded window
- [x] 4.4 Write `scripts/audio_capture.py`: parse `[AUDIO_WAVE]` chunks (decimated + burst modes), reassemble into a WAV, and assert features — glitch `|Δsample|` spikes, RMS envelope, zero-crossing rate, FFT signatures
- [x] 4.5 Build with `-DAUDIO_DEBUG` and validate a real on-device capture (stats lines, self-test sine peak, glitch-free run)

## 5. Layer E — Golden Metric Regression

- [x] 5.1 Record the golden metric profile (RMS bins, ZCR, FFT peaks) from a reference capture and store as golden JSON
- [x] 5.2 Write the comparison tooling (relative tolerances + minimum-value pins) and wire it into `scripts/audio_capture.py` or a sibling script; exit nonzero on deviation
- [x] 5.3 Validate the golden set catches a regression (e.g. reduce a voice volume in a scratch build and confirm the run fails)

## 6. Final Validation

- [x] 6.1 Confirm production builds are unaffected: `pio run -e TRACKLINK_V3` and `pio run -e MIKRO_V2` without `AUDIO_DEBUG`
- [x] 6.2 Run `scripts/validate_sounds.py` clean, and run the extended `scripts/smoke_test.py` end-to-end with the panic watch enabled
- [x] 6.3 Document the workflow (script usage, build flags, self-test commands) in the repo README or `docs/`
