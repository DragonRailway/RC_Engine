## 1. Power-Off Dispatch & Charging Mode Teardown

- [x] 1.1 Implement `stopOperationalStackToCharging()` in `src/main.cpp` to tear down BLE, mute/stop audio output, stop motors, sleep servos, and transition to Minimal Charging Mode with `POWER_ENABLE` latched
- [x] 1.2 Update power button hold detection in `HardwareInit` to allow `controlTask` to dispatch shutdown to `stopOperationalStackToCharging()` when USB is connected vs `HardwareInit::powerOff()` when on battery
- [x] 1.3 Update `VehicleController::update()` disconnect timeout to transition to charging mode if USB is connected

## 2. Verification and Testing

- [x] 2.1 Add Test 32 in `test/host_vc/host_vc_driver.cpp` to verify USB shutdown into Charging Mode with `POWER_ENABLE` latched and subsequent wake to Run Mode
- [x] 2.2 Run host tests (`python3 scripts/host_vc_test.py`) and build firmware across board environments (`MIKRO_V2`, `TRACKLINK_V2`, `TRACKLINK_V3`)
