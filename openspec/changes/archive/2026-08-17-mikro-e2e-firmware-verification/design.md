## Context

The `MIKRO_V2` board is an ESP32-S3-based micro RC vehicle controller connected via USB CDC (`/dev/ttyACM0`) to the host computer and communicating with an Android companion app over Bluetooth Low Energy (BLE). The companion app exposes a local REST API (`127.0.0.1:17007` via adb forward) allowing programmatic widget manipulation (steering wheel, throttle slider, brake pedal, gear switch, start/stop toggle, lighting selector, horn, and aux slider).

## Goals / Non-Goals

**Goals:**
- Provide clear real-time serial status logging in the firmware so host test harnesses and human developers can observe servo angle, motor PWM, light bitmask, gear, and engine RPM.
- Verify DSP audio timing (< 2900 µs buffer computation limit) and sample generation stability under real-time load.
- Provide a unified end-to-end Python test script (`scripts/verify_mikro_e2e.py`) that handles firmware compilation, device flashing, LittleFS config flashing (`hardware-MIKRO_V2-truck.json` + `ScaniaV8`), and remote REST API driving + serial assertion.

**Non-Goals:**
- Modifying UI layouts or creating new BLE widgets in the Android app.
- Altering the fundamental sound engine synthesis math or PCM file formats.

## Decisions

1. **Dual Telemetry Strategy in Firmware**:
   - *Decision*: Combine event-driven logging (immediate print on gear change, engine state transition, or lighting mode toggle) with a 250ms periodic rate-limited status line when values are active.
   - *Rationale*: Event logs capture fast transient transitions; periodic logs provide continuous visibility without spamming the serial link at loop frequency (100Hz+).

2. **Unified E2E Runner Architecture**:
   - *Decision*: Consolidate flashing, FS bundle staging (`scripts/build_fs.py`), and REST API widget sequencing into `scripts/verify_mikro_e2e.py` with multi-threaded or timed serial log interception.
   - *Rationale*: Avoids juggling separate terminal windows for monitoring, flashing, and app driving.

3. **Audio Performance Assertion**:
   - *Decision*: Parse `[AUDIO_STATS]` lines emitted by `AudioOutput.h` / `SoundEngine` to assert buffer computation duration is strictly less than the 2900 µs real-time safety threshold and verify zero Panics.

## Risks / Trade-offs

- [Serial buffer saturation at 2,000,000 baud] → Rate limit periodic telemetry to 250ms and keep message formatting compact.
- [App-BLE reconnection latency after board reboot/flash] → Allow a 5–10s settling window with status polling before firing REST API commands.
