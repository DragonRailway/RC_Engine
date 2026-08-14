# Audio Debug Tooling Guide

This document describes the 5-layer audio verification system for `RC_brain`.

## Architectural Layers

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          AUDIO DEBUG TOOLING                            │
├─────────────────────────────────────────────────────────────────────────┤
│ Layer A: Off-Device Sound Data Validator (scripts/validate_sounds.py)   │
│ Layer B: Panic Watch in Smoke Test (scripts/smoke_test.py)              │
│ Layer C: Host DSP Harness & Tests (scripts/host_dsp_test.py)           │
│ Layer D: On-Device Telemetry & Waveform Capture (audio_capture.py)      │
│ Layer E: Golden Metric Profile Regression (scripts/golden_metrics.py)   │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 1. Off-Device Sound Data Validation (Layer A)

Validates all vehicle bundles under `configs/vehicle_configs/`:
- Bundle layout: bundle dir name == `sound_set` in its `vehicle.json`; every bundle has a `sounds/` subdirectory
- Sample rate consistency (22,050 Hz)
- Non-empty sample count & array length match
- Loop point bounds (`0 <= begin < end <= count`)
- RMS, peak, DC offset, silence, and clipping analysis

```bash
python3 scripts/validate_sounds.py
```

---

## 2. Panic Watch in E2E Smoke Test (Layer B)

Monitors 2 Mbaud serial output during boot log drain and widget exercises for panic regex matches (`Guru Meditation`, `Coprocessor exception`, `abort()`, `Backtrace:`).

```bash
python3 scripts/smoke_test.py
```

---

## 3. Host DSP Unit Harness (Layer C)

Compiles the C++ engine (`lib/SoundEngine/src/RcEngineSound.cpp`) natively on x86 against a lightweight `Arduino.h` stub.

Tests:
- Pitch tracking vs RPM / throttle ramping (Zero-Crossing Rate analysis)
- Loop region boundary enforcement
- One-shot voice deactivation
- Int8 accumulator overflow and non-finite (`NaN`/`Inf`) checks
- Regression detection (`--break-loop`)

```bash
python3 scripts/host_dsp_test.py
```

---

## 4. On-Device Instrumentation & Waveform Capture (Layer D)

Build firmware with `-DAUDIO_DEBUG` in `platformio.ini` or build command:

```ini
build_flags =
    -DAUDIO_DEBUG
```

Emits structured serial lines:
- `[AUDIO_STATS]`: Per-buffer peak, RMS, clip count, task latency ($\mu s$), I2S write delay ($\mu s$)
- `[AUDIO_VOICE]`: Voice snapshot (active, position, count, volume)
- Serial self-test commands (`AUDIO_SELFTEST <sine|impulse|sweep|silence>`)

Run waveform feature analysis & WAV capture:
```bash
python3 scripts/audio_capture.py --file serial_log.txt --wav output.wav
```

---

## 5. Golden Metric Profile Regression (Layer E)

Compares current capture metrics against golden metric JSON profiles using relative tolerances (RMS $\pm 15\%$, ZCR $\pm 10\%$) and minimum energy pins.

```bash
# Record reference golden profile
python3 scripts/golden_metrics.py --record

# Compare current run against golden profile
python3 scripts/golden_metrics.py

# Verify regression detection
python3 scripts/golden_metrics.py --test-regression
```
