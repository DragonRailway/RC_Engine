## Context

Audio generation has two halves with different failure profiles:
- **DSP math** (`RcEngineSound::getNextSample`): deterministic given voice state — fractional interpolation, loop regions, int8 mixing, knock cadence. Testable off-device, but currently untested.
- **Realtime pipeline** (`AudioOutput`): 22,050 Hz timer ISR notifying a pinned audio task that writes 64-sample int16 buffers to I2S. Failure modes here are timing/race-shaped: the FPU-in-ISR coprocessor panic (fixed in `8f43362`), spinlocked `voices[]` snapshot/write-back, blocking I2S writes, buffer pacing.

Existing assets: `scripts/smoke_test.py` (widget-level E2E over 2 Mbaud serial, no audio checks), `scripts/capture_serial.py` / `capture_boot.py` (serial capture pattern), vendored `lib/SoundEngine` (RcEngineSound + AudioOutput, thin `Arduino.h` surface for the engine), sound data as JSON under `data/sounds/`.

## Goals / Non-Goals

**Goals:**
- Verify sound *data* integrity off-device (loop bounds, rates, silence/clipping).
- Catch panic-class regressions (ISR/FPU) automatically in the E2E smoke test.
- Test the DSP deterministically on the host without hardware.
- Measure and stream the realtime pipeline from the device (buffer stats, voice activity, timing) and turn the stream into auditable waveform features.
- Provide golden metric regression so behavior drift is caught over time.
- Keep every instrument behind a build flag or separate script — zero impact on production firmware.

**Non-Goals:**
- No changes to the production audio path or DSP algorithms (we verify, not refactor).
- No audio playback on the PC via line-out; capture is serial-based.
- No new sound content or vehicle profiles.
- No continuous-integration server setup — scripts are runnable and CI-ready, not wired into a hosted CI.

## Decisions

### D1: Test-pyramid placement
```
Off-device (no hardware, fast)          On-device (hardware, slow)
──────────────────────────────          ─────────────────────────────
A. validate_sounds.py                   B. smoke_test.py (+ panic watch)
C. host_dsp_test.py  ◀──────────────▶   D. audio_capture.py (AUDIO_DEBUG)
E. golden metrics (consumes D capture, also off-device analysis)
```
Ordering: A and C run before flashing (fast feedback); B runs the standard E2E; D/E run on demand for deep audio verification. All scripts exit nonzero on failure so they can gate any future CI or pre-release workflow.

### D2: Host DSP harness — compile the *real* engine
`host_dsp_test.py` does not reimplement the DSP (reimplementation drifts). Instead a small host build compiles the actual `lib/SoundEngine/src/RcEngineSound.cpp` against a stub `Arduino.h` (millis, no-op Serial, `portMUX_TYPE` + `portENTER_CRITICAL/EXIT_CRITICAL` as no-ops, `random()` for the candle path). The test binary accepts a script (start → rev ramp → horn → gear → stop) and emits raw int8 PCM plus per-voice activity logs; Python asserts features on the output.
- *Alternative considered:* PlatformIO `native` env. Rejected as primary because LDF pulls EasyKit/RadioKit deps; a standalone Makefile-style compile of the single engine TU + stubs keeps the harness minimal. (A PIO `native` env may be added later.)
- *Risk:* `SoundTypes.h`/`VehicleProfile.h` may drag in more Arduino surface than expected. → Constrain includes to the minimum (stub anything undefined), iterate until the engine TU compiles clean on x86; this is task-1 of the harness and must gate the rest.

### D3: Instrumentation split — cheap always, deep gated
- **Buffer stats (cheap, integer-only, in `audioTask`)**: peak, RMS, clip count, DC offset per 64-sample buffer, plus one `esp_timer_get_time()` per buffer to measure total task-loop time and `i2s_channel_write` blocking time. `AUDIO_DEBUG` gates the emit, not the counters.
- **DSP internals (gated)**: in `getNextSample`, `#ifdef AUDIO_DEBUG` checks `isfinite()` on mix accumulators and a voice-activity snapshot accessor (`debugVoiceSnapshot()` → active/position/count/loop bounds per voice), sampled once per second from the main loop.
- **No per-sample `esp_timer_get_time()` in `getNextSample`** — too hot; per-buffer timing in the task gives the same signal.

### D4: Serial stream format and bandwidth
Structured lines (existing capture pattern):
```
[AUDIO_STATS] {"peak":32767,"rms":8120,"clips":0,"nan":0,"task_us":920,"i2s_us":340}
[AUDIO_VOICE] {"id":3,"active":1,"pos":1234.5,"count":8192,"vol":80}
[AUDIO_WAVE] <raw int16 samples as hex/binary chunk>
```
Bandwidth math: 64 samples × 2 B = 128 B/buffer; full-rate dumping = ~2.8 MB/s, far above 2 Mbaud (~250 KB/s). Two modes therefore:
- **Decimated capture** (default): dump 1 buffer per N (N≈100 → ~440 B/s) for long, metric-grade capture.
- **Burst capture** (command-triggered): buffer up to a bounded window (e.g. 1 s = 44 k samples = 88 KB in PSRAM) and dump at full rate for glitch/RPM analysis.
Waveform features (glitch `|Δ|` spikes, RMS envelope, zero-crossing rate, FFT) are computed on the reconstruction; decimation at N=100 still preserves envelope and ZCR for engine tones.

### D5: Self-test signals to isolate pipeline vs. engine
A serial command (`AUDIO_SELFTEST <sine|impulse|sweep|silence>`) switches the audio task's source from `engine->getNextSample()` to a debug signal generator for a bounded window. If sine 440 Hz shows a clean 440 Hz FFT peak with zero glitches, the pipeline (task pacing, I2S, ramp) is healthy and any anomaly in normal mode is attributable to engine logic or voice state.

### D6: Golden metrics — feature profiles, not samples
Store per-phase metric profiles (RMS envelope binned at 100 ms, ZCR per phase, FFT peak frequencies) as JSON goldens. Re-run comparison uses relative tolerances (e.g. RMS ±15%, ZCR ±10%) plus monotonicity/ordering checks (rev-up then rev-down). Sample-exact comparison is rejected: main-loop timing jitter makes it flaky; feature profiles are stable.
- *Trade-off:* Tolerances can mask small regressions → golden set also pins *minimum* values (e.g. "horn phase must contain a spectral peak at the horn's base frequency") so a regression that reduces energy still fails.

### D7: Panic watch in the smoke test
A shared regex list (`Guru Meditation|Coprocessor exception|abort()|Backtrace`) asserted (a) during the existing 12 s boot drain and (b) after each widget phase. Zero new hardware or protocol work; reuses the existing read loop. The FPU crash prints "Coprocessor exception" on this IDF build — the exact regression class is covered.

## Risks / Trade-offs

- **Host-build friction** (Arduino surface in engine TUs) → D2 mitigates: stub aggressively, gate harness tasks on clean x86 compile; if a TU is too entangled, move that check into a device-side `pio test` case instead of blocking the rest.
- **Serial bandwidth vs. fidelity** → D4 dual mode: decimated for long metrics, burst for detailed analysis. Feature math chosen to tolerate decimation.
- **Instrumentation overhead skewing timing** → Counters are integer math; only the *emit* is gated. Timing figures measured in `AUDIO_DEBUG` include emit overhead → report task-loop time excluding the debug emit, and treat absolute numbers as indicative, relative deltas as signal.
- **Golden metric drift over firmware iterations** → Tolerances + minimum-value pins (D6); regenerate goldens deliberately with a flag, never silently.
- **Audio stats flooding serial in normal use** → `AUDIO_DEBUG` is off in production builds; default decimation keeps debug builds talkative but bounded.

## Migration Plan

1. Land Layers A + B first (pure scripts, zero firmware impact) — immediate value, no risk.
2. Land Layer C harness; verify it catches a known regression (e.g. temporarily break a loop bound in a scratch copy and confirm the test fails).
3. Land Layer D firmware instrumentation behind `-DAUDIO_DEBUG`; validate capture on-device, then Layer E goldens.
4. Rollback: remove the build flag / revert script commits; production firmware is untouched throughout.

## Open Questions

- Should Layer C eventually run as a PlatformIO `native` env (`pio test -e native`) for a single-command `pio test` workflow, or stay a standalone script? Standalone chosen for v1; revisit if the project standardizes on `pio test`.
- Burst-capture window size (1 s assumed; verify PSRAM headroom on the loaded config).
- Is a hosted CI workflow wanted for A/C (would make them true gates)? Out of scope until the repo has a CI runner.
