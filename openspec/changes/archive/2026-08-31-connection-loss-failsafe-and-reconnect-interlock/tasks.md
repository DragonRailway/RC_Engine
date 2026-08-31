## 1. Hardware Infrastructure & Actuator Methods

- [x] 1.1 Add `HardwareInit::detachServos()` and `HardwareInit::attachServos()` in `common/HardwareInit.h` and `common/HardwareInit.cpp` to safely depower/re-power steering servos.
- [x] 1.2 Add `HardwareInit::setPump(bool)` helper in `common/HardwareInit.h` / `common/HardwareInit.cpp` using `BOARD::POWER::PUMP_ENABLE`.

## 2. VehicleController Disconnect Failsafe & Interlock State Machine

- [x] 2.1 Update `common/VehicleController.h` with connection loss tracking state (`s_reconnectThrottleInterlock`, `s_disconnectBraking`, `s_disconnectEngineStopDone`).
- [x] 2.2 Implement 50% braking stop, servo detach, aux motor shutdown, and pump shutdown in `common/VehicleController.cpp` when `!RadioKit.isConnected()`.
- [x] 2.3 Implement immediate idle sound drop, automatic 4-way hazard lights activation, and 30-second engine shutdown during disconnect in `common/VehicleController.cpp`.
- [x] 2.4 Implement reconnect throttle-to-neutral safety interlock and servo re-attachment upon reconnection in `common/VehicleController.cpp`.

## 3. Host Tests & Verification

- [x] 3.1 Add comprehensive host test assertions in `test/host_vc/host_vc_driver.cpp` (Test 27) covering:
  - 50% braking stop upon connection drop
  - Steering servo detach & aux motor shutdown
  - 4-way hazard activation
  - 30-second engine stop sequence
  - Reconnect throttle-to-neutral interlock (verifying throttle stays locked at 0 until neutral zero-crossing)
- [x] 3.2 Validate configs with `python3 scripts/validate_configs.py` and run host test harness.
- [x] 3.3 Verify full PlatformIO build across all environments (`pio run`).
