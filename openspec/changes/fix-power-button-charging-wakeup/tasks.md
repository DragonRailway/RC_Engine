## 1. Power Button Dispatch Refactoring

- [x] 1.1 Add `VehicleController::handlePowerButtonClick()` to `common/VehicleController.h` and implement it in `common/VehicleController.cpp` to reset `s_disconnectStart` and `s_inWarningPhase`.
- [x] 1.2 Remove the redundant `HardwareInit::consumeButtonClicked()` call from inside `VehicleController::update()`.
- [x] 1.3 In `src/main.cpp` (`controlTask()`), call `HardwareInit::update()` and consume `HardwareInit::consumeButtonClicked()` once per cycle, dispatching to `startOperationalStack()` if in charging mode and always forwarding to `VehicleController::handlePowerButtonClick()`.

## 2. Quiescent Charging Mode Gate

- [x] 2.1 In `common/VehicleController.cpp`, ensure the quiescent charging mode block is guarded by `HardwareInit::isUsbBoot()`, allowing normal operational execution when the board is switched ON while docked.
- [x] 2.2 Verify and adjust `test/host_vc` mock harness if needed so host unit tests pass.

## 3. Verification & Build

- [x] 3.1 Run host tests via `make -C test/host_vc run` or PlatformIO test runner.
- [x] 3.2 Build firmware for `TRACKLINK_V2` via `pio run -e TRACKLINK_V2`.
- [x] 3.3 Verify board telemetry over serial after build/upload.
