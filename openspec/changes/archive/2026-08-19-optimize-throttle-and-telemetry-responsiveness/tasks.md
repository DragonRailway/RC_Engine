# Tasks: Optimize Throttle and Telemetry Responsiveness

## 1. Sound Engine and Vehicle Profile Responsiveness

- [x] 1.1 Update `lib/SoundEngine/src/RcEngineSound.cpp` to scale `virtualSpeed` acceleration and deceleration steps (`max(4, cfg.engine.acc * 4)`), accelerating multi-gear automatic transmission transitions
- [x] 1.2 Update `configs/vehicle_configs/ScaniaV8/vehicle.json` engine acceleration from 2 to 6 and deceleration from 1 to 4 for responsive truck dynamics

## 2. Telemetry Cadence Optimization

- [x] 2.1 Update `common/VehicleController.h` `update()` to publish telemetry at a 100ms interval (`now - s_lastTelemetry >= 100`)

## 3. Verification and Testing

- [x] 3.1 Run host DSP tests (`python3 scripts/host_dsp_test.py`) and host vehicle controller tests (`python3 scripts/host_vc_test.py`)
- [x] 3.2 Build firmware for `MIKRO_V2` and `TRACKLINK_V3` targets (`pio run`)
- [x] 3.3 Deploy filesystem bundle to board and verify live on-device throttle step response
