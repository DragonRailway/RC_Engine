## Context

During RC vehicle operation from the RadioKit mobile/tablet control UI, touch inputs on `gas_pedal` must produce immediate, snappy vehicle response. The user observed a noticeable lag. Diagnosis revealed:
1. `RcEngineSound.cpp` automatic transmission inertia: `virtualSpeed += max(1, cfg.engine.acc * 2)` only stepped +4 RPM per update. With `maxRpm = 500` and 3 gears, reaching top gear took 126 updates (~1.3s delay).
2. Telemetry publication in `VehicleController.h` was throttled to 250ms (4 Hz), making UI gauges and speed telemetry laggy.
3. Vehicle profile parameters in `ScaniaV8` can be tuned for responsive acceleration while preserving realistic multi-gear shifting sound.

## Goals / Non-Goals

**Goals:**
- Accelerate virtual speed step rate in `RcEngineSound.cpp` so automatic transmission vehicles rev up briskly without losing shift sounds.
- Update `ScaniaV8` vehicle profile acceleration to provide responsive throttle feel.
- Reduce telemetry update interval in `VehicleController.h` from 250ms to 100ms (10 Hz).
- Maintain all existing safety interlocks (Park lock, two-tier battery protection, out-of-fuel sound).

**Non-Goals:**
- Rewriting the BLE transport library in `rk-arduino`.
- Altering the physical motor PWM duty mapping formulas.

## Decisions

### 1. Enhanced Automatic Transmission Step Rate
- **Decision**: In `RcEngineSound.cpp`, scale the `virtualSpeed` acceleration/deceleration step:
  ```cpp
  int32_t step = max((int32_t)4, (int32_t)(cfg.engine.acc * 4));
  ```
  And tune `acceleration` in `ScaniaV8/vehicle.json` from `2` to `6`.
- **Rationale**: This reduces the ramp time from 126 update cycles (~1.3s) down to ~25 update cycles (~250ms), giving snappy throttle response while still cleanly playing the 1st -> 2nd -> 3rd gear shift audio transitions.

### 2. 10 Hz Telemetry Publication
- **Decision**: Change `if (now - s_lastTelemetry >= 250)` to `if (now - s_lastTelemetry >= 100)` in `VehicleController.h`.
- **Rationale**: 100ms provides smooth 10 fps speedometer and gauge animations on the tablet without flooding the BLE link.

## Risks / Trade-offs

- [Risk] Faster audio RPM transitions could compress shift sounds into very short bursts.
  → Mitigation: The gear size and shifting voice triggers remain intact; 250ms gives plenty of time for audible shift articulation while feeling instantaneous to the user.
