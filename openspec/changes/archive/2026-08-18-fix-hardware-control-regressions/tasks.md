## 1. Pedal Normalization & Power Latch

- [x] 1.1 Update `VehicleController::update()` throttle calculation: map `gas_pedal.rk.value` from `[-100, +100]` to `[0, 100]%` via `(val + 100) / 2` when in truck mode
- [x] 1.2 Update `brake_pedal` normalization from `[-100, +100]` to `[0, 100]%` via `(val + 100) / 2` and adjust deadband logic
- [x] 1.3 Add power-rail re-assertion on active BLE connection in `VehicleController::update()`

## 2. Turn Signal App-Suppression State Machine

- [x] 2.1 Add `s_leftIndSuppressed` and `s_rightIndSuppressed` state tracking in `VehicleController.h`
- [x] 2.2 Implement auto-cancel suppression logic: set suppression on cancel, ignore `state == true` while suppressed, clear suppression when `state == false`
- [x] 2.3 Refine turn indicator steering baseline and center return cancellation logic

## 3. HardwareInit LEDC Channel Consolidation

- [x] 3.1 Verify and consolidate `HardwareInit::initLights()` LEDC channel allocation on ESP32-S3 (8 channels max)
- [x] 3.2 Ensure aliased pins (`reversingLight` on `brakeLight`) do not allocate duplicate channels

## 4. Host Unit Tests & Device Verification

- [x] 4.1 Update `test/host_vc/host_vc_driver.cpp` with unit tests for pedal mapping `[-100, 100]` and turn signal suppression
- [x] 4.2 Run host tests and verify clean pass
- [x] 4.3 Build firmware for `MIKRO_V2`, flash over USB, and run full Remote API hardware test suite
