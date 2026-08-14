# Audio Debug Tooling

## Purpose

Repository-level tooling for validating and debugging the audio pipeline: off-device sound data validation, panic detection in the end-to-end smoke test, host-side DSP testing of the real engine code, an on-device `AUDIO_DEBUG` instrumentation/capture mode, and golden-metric regression comparison.

## Requirements

### Requirement: Off-device sound data validation
The repository SHALL provide `scripts/validate_sounds.py`, which validates every sound JSON under `configs/vehicle_configs/` (bundle layout: dir name equals the `sound_set` in the bundle's `vehicle.json`, every bundle has a `sounds/` subdirectory) — `sampleRate` consistency (22,050 Hz), positive `sampleCount`, loop-point bounds (`0 ≤ begin < end ≤ count` for every voice with loop points), and per-slot signal stats (RMS, peak, DC offset, silence, clipping) — and SHALL exit with a nonzero code when any file fails.

#### Scenario: Loop point out of bounds
- **WHEN** a sound JSON declares `hornBegin`/`hornEnd` where `hornEnd > sampleCount`
- **THEN** `validate_sounds.py` reports the offending file and exits nonzero

#### Scenario: Sample rate mismatch
- **WHEN** a sound JSON declares a `sampleRate` other than 22,050
- **THEN** `validate_sounds.py` reports the file and exits nonzero

#### Scenario: All sound data valid
- **WHEN** every sound JSON passes the checks
- **THEN** `validate_sounds.py` exits zero and prints a per-file summary

### Requirement: Panic detection in end-to-end smoke test
`scripts/smoke_test.py` SHALL assert that no panic pattern (Guru Meditation, coprocessor/FPU exception, abort, backtrace) appears in the serial output during boot or during the widget exercise phases, and SHALL report a FAIL and exit nonzero if one does.

#### Scenario: Clean run
- **WHEN** boot and the full widget exercise produce no panic strings
- **THEN** the smoke test reports PASS for the panic check and continues

#### Scenario: Panic during exercise
- **WHEN** a coprocessor exception string appears in serial during the exercise
- **THEN** the smoke test reports FAIL and exits nonzero

### Requirement: Host-side DSP testing
The repository SHALL provide a host test harness (`scripts/host_dsp_test.py` plus a stub-Arduino native build of `RcEngineSound`) that compiles and runs the real engine code on the host, drives deterministic scripts, and asserts on the generated sample stream: pitch via zero-crossing rate matches the commanded RPM/pitch factor, loop regions are respected, one-shot voices deactivate exactly once, knock cadence matches the configured pattern, and no NaN or int8 overflow occurs.

#### Scenario: Pitch tracks RPM
- **WHEN** a script commands a known throttle/RPM ramp while the engine is running
- **THEN** the zero-crossing rate of the generated idle/rev stream scales with the commanded RPM within tolerance

#### Scenario: Loop region respected
- **WHEN** a looped voice (e.g. horn) with defined loop points is active
- **THEN** the generated samples never advance past the declared loop end

#### Scenario: One-shot deactivates
- **WHEN** a one-shot voice (e.g. start or shifting sound) finishes
- **THEN** its voice deactivates and contributes silence on subsequent samples

#### Scenario: No NaN or overflow
- **WHEN** any test script runs to completion
- **THEN** no generated sample is NaN and no int8 mixing accumulator overflows

### Requirement: On-device audio instrumentation and capture
The firmware SHALL provide an `AUDIO_DEBUG` build mode that emits structured serial lines — per-buffer peak/RMS/clip/NaN stats, per-second voice activity (active/position/count), and audioTask timing (task-loop time, I2S write blocking time) — and SHALL support a serial-triggered self-test mode that plays known test signals (sine, impulse, sweep, silence) through the normal pipeline. `scripts/audio_capture.py` SHALL reassemble captured buffers into a WAV and assert waveform features: glitch spikes, RMS envelope shape, zero-crossing rate, and FFT signatures.

#### Scenario: Stats emitted in debug mode
- **WHEN** firmware is built with `AUDIO_DEBUG` and audio is active
- **THEN** structured `[AUDIO_STATS]` and `[AUDIO_VOICE]` lines appear on serial at bounded rates

#### Scenario: Self-test signal verified
- **WHEN** the sine self-test signal is requested and the capture is analyzed
- **THEN** the reconstructed waveform has a dominant FFT peak at the sine's frequency and no glitch spikes above threshold

#### Scenario: Glitch detection
- **WHEN** captured samples contain a sample-to-sample jump above the configured threshold
- **THEN** the analyzer flags the glitch and the run fails the glitch assertion

### Requirement: Golden metric regression
The repository SHALL store per-phase metric profiles (RMS envelope binned at 100 ms, zero-crossing rate, FFT peak frequencies) as golden JSON, SHALL compare re-runs against them with relative tolerances plus minimum-value pins, and SHALL exit nonzero when any metric deviates beyond tolerance.

#### Scenario: Regression detected
- **WHEN** a re-run's RMS envelope falls below the golden minimum for a phase
- **THEN** the comparison reports a FAIL and exits nonzero

#### Scenario: Stable run passes
- **WHEN** a re-run's metrics stay within tolerance of the golden profile
- **THEN** the comparison passes and exits zero
