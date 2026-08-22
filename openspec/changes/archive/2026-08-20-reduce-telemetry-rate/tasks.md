# Tasks: Reduce Telemetry Rate

## 1. Firmware Change

- [x] 1.1 Change `s_lastTelemetry >= 100` to `s_lastTelemetry >= 1000` in `common/VehicleController.h` (line ~866 in `update()`)

## 2. Verification

- [x] 2.1 Build both environments (MIKRO_V2, TRACKLINK_V3) and confirm clean compilation
- [x] 2.2 Run host VC tests (`python3 scripts/host_vc_test.py`) and confirm all pass
- [x] 2.3 Flash to MIKRO board and verify telemetry updates at 1Hz via serial monitor
