## 1. Hardware Driver Layer

- [x] 1.1 Update `common/HardwareInit.h` to declare `static void sleepServos()` and `static void wakeServos()`, removing legacy `detachServos()` and `attachServos()`.
- [x] 1.2 Implement `HardwareInit::sleepServos()` and `HardwareInit::wakeServos()` in `common/HardwareInit.cpp` to sleep and wake all steering servos and aux servos.
- [x] 1.3 Update `HardwareInit::stopAll()` in `common/HardwareInit.cpp` to call `.stop()` and `.sleep()` on steering servos, aux servos, and ESC servos instead of calling `.detach()`.

## 2. Vehicle Controller Failsafe Integration

- [x] 2.1 Update `common/VehicleController.cpp` to call `HardwareInit::sleepServos()` upon controller disconnect failsafe.
- [x] 2.2 Update `common/VehicleController.cpp` to call `HardwareInit::wakeServos()` upon controller reconnection.

## 3. Host Tests and Verification

- [x] 3.1 Update `test/host_vc/host_easykit_stubs.cpp` to support `EasyServo::sleep()` and `EasyServo::wake()` tracking.
- [x] 3.2 Update `test/host_vc/host_vc_driver.cpp` test assertions to verify servo sleep on disconnect and wake on reconnect.
- [x] 3.3 Build and run host test suite (`host_vc`) and PlatformIO compilation to verify zero regressions.
