## Why

ESP-IDF's MCPWM driver exhibits known stability issues, handle churn, and potential panics when drivers/operators/generators are repeatedly created and destroyed via `detach()` and `attach()` during runtime events (such as controller disconnect/reconnect cycles or teardown). Using `EasyServo::sleep()` and `EasyServo::wake()` immediately forces PWM output LOW to depower servos without destroying underlying hardware peripheral resources, ensuring rock-solid stability and zero re-initialization latency.

## What Changes

- **Firmware Servo Lifecycle (`HardwareInit`)**:
  - Replace `HardwareInit::detachServos()` and `HardwareInit::attachServos()` with `HardwareInit::sleepServos()` and `HardwareInit::wakeServos()`.
  - In `sleepServos()`, call `.sleep()` on all active steering servos and auxiliary servos.
  - In `wakeServos()`, call `.wake()` on all active steering servos and auxiliary servos.
  - In `stopAll()` (called during board shutdown / powerOff), replace `.detach()` calls on steering servos, aux servos, and ESC servos with `.stop()` and `.sleep()`.
- **Failsafe & Reconnect Integration (`VehicleController`)**:
  - Call `HardwareInit::sleepServos()` on controller disconnection failsafe.
  - Call `HardwareInit::wakeServos()` on controller reconnection.
- **Host Test Harness (`test/host_vc/`)**:
  - Update `host_easykit_stubs.cpp` to support `EasyServo::sleep()` and `EasyServo::wake()` tracking.
  - Update `host_vc_driver.cpp` assertions to verify servo sleep on disconnect and wake on reconnect.

## Capabilities

### New Capabilities
<!-- None -->

### Modified Capabilities
- `vehicle-control-loop`: Update connection loss failsafe and reconnection requirements to specify servo sleeping/waking instead of detaching/attaching.
- `board-power-management`: Update board power teardown and failsafe requirements to use servo sleeping instead of detaching.

## Impact

- **Affected Code**: `common/HardwareInit.h`, `common/HardwareInit.cpp`, `common/VehicleController.cpp`, `test/host_vc/host_easykit_stubs.cpp`, `test/host_vc/host_vc_driver.cpp`.
- **APIs**: `HardwareInit::sleepServos()` and `HardwareInit::wakeServos()` replace `detachServos()` and `attachServos()`.
- **Dependencies**: No change to `ESP32_EasyKit` or external libraries; purely firmware-level usage of existing EasyKit `sleep()` and `wake()` APIs.
