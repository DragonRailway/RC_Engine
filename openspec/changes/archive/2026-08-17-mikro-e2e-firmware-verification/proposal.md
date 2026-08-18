## Why

To guarantee the reliability, performance, and safety of the RC Brain firmware on the physical `MIKRO_V2` hardware with the `ScaniaV8` vehicle bundle, we need an automated, repeatable end-to-end verification pipeline. This verifies flashing, LittleFS config deployment, real-time actuator control (servo, motor, lights), sound engine DSP performance, and remote control synchronization via the RadioKit companion app REST API over USB CDC and BLE.

## What Changes

- **Firmware Real-Time Serial Telemetry**: Add non-blocking, periodic & event-driven serial debug telemetry in `VehicleController` reporting engine state, RPM, gear selection, throttle %, motor PWM %, steering angle, and active lighting states.
- **Sound Engine Audio Verification**: Add DSP buffer timing and audio statistic monitoring assertions (< 2900 µs buffer computation, no clipping or NaN math) during live engine simulation.
- **Automated End-to-End Test Suite**: Create a unified automated test runner (`scripts/verify_mikro_e2e.py`) that flashes firmware, builds/deploys the `ScaniaV8` LittleFS bundle, drives the RadioKit Android App Remote REST API (127.0.0.1:17007), and asserts responses over USB serial.

## Capabilities

### New Capabilities
- `mikro-e2e-verification`: Unified hardware, sound engine, and remote API end-to-end verification pipeline for the MIKRO_V2 controller with ScaniaV8 profile.

### Modified Capabilities
<!-- None -->

## Impact

- **Firmware**: `common/VehicleController.h` and `src/main.cpp` gain enhanced serial debug telemetry hooks.
- **Host Tooling**: Adds `scripts/verify_mikro_e2e.py` for comprehensive integration testing.
- **Dependencies**: Uses PlatformIO, Python `pyserial`, `requests`/`urllib`, and LittleFS flashing tools.
