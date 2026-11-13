## Why

When an ESP32-S3 board (such as TRACKLINK_V2) boots while connected to USB, it enters minimal charging mode (`s_operationalMode = false`) with the BLE modem and audio output disabled. The design specifies that pressing the power button exits charging mode and transitions the board to `ON` (`startOperationalStack()`).

However, pressing the power button currently fails to exit charging mode due to two defects:
1. **Event Double-Consumption**: In `controlTask()`, `VehicleController::update()` executes first and consumes `HardwareInit::consumeButtonClicked()`, resetting the flag to `false`. When `controlTask()` checks `consumeButtonClicked()` immediately afterwards, the event is already gone, so `startOperationalStack()` is never called.
2. **Docked State Re-entry**: Even if operational mode were started, `VehicleController::update()` unconditionally re-evaluates USB presence (`HWCDC::isPlugged()` or `batV > 4.35V`) and forces `s_chargingState = CHARGING_COMPLETE` whenever `!isConnected`, immediately early-returning and suppressing normal operation.

Fixing this ensures vehicles can be charged via USB and reliably switched ON with the physical power button to drive or connect without having to unplug USB.

## What Changes

- **Reliable Button Click Dispatch**:
  - Unify button click consumption in `controlTask()` so the click event is captured once per cycle and forwarded to both `VehicleController` (to reset the disconnect timer) and `main.cpp` (to switch ON from minimal charging mode).
- **Persistent ON State**:
  - Provide a mechanism for `VehicleController` to respect the `ON` state so that switching ON while plugged into USB keeps the operational stack active (advertising BLE, running audio, and allowing control) rather than forcing it back into quiescent charging mode while waiting for connection.
- **Diagnostics & Status Logging**:
  - Update serial telemetry or status messages to clearly distinguish between `[CHARGING_ACTIVE]`, `[CHARGING_COMPLETE]`, and `[ON (Docked)]` states.

## Capabilities

### Modified Capabilities
- `board-power-management`: Ensure clicking the power button reliably transitions the board from `CHARGING_ACTIVE` or `CHARGING_COMPLETE` to `ON` even while connected to USB power.

## Impact

- Affected files:
  - `src/main.cpp`: Event consumption and operational state dispatch.
  - `common/VehicleController.h` / `common/VehicleController.cpp`: Respect manual `ON` transition when USB is plugged in; remove redundant internal `consumeButtonClicked()` call.
  - `common/HardwareInit.h`: Add non-destructive `isButtonClicked()` / clear methods or keep `consumeButtonClicked()` scoped to the main control loop.
