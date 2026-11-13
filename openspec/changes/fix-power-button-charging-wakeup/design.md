## Context

ESP32-S3 boards (such as TRACKLINK_V2) feature a hardware power latch circuit, power button input, and battery charge detection. When booted via USB power, the board detects `digitalRead(POWER_BUTTON) == LOW` during `HardwareInit::latchPower()`, setting `s_isUsbBoot = true`.

To conserve energy and prevent heat accumulation while charging, the board boots into a minimal charging mode where the BLE modem and audio synthesis are inactive until the user presses the power button to switch ON.

However, in practice:
1. `HardwareInit::consumeButtonClicked()` was called inside `VehicleController::update()` on each 20ms tick. Because `controlTask()` executed `VehicleController::update()` before checking `consumeButtonClicked()`, the single-click flag was cleared prematurely, making it impossible for `main.cpp` to see the button press.
2. `VehicleController::update()` gated the charging quiescent state on `s_chargingState != ChargingState::NOT_CHARGING && !isConnected`. Because USB presence (`HWCDC::isPlugged()` or $V_{\text{bat}} > 4.35\text{V}$) keeps `s_chargingState` at `CHARGING_COMPLETE` even after `startOperationalStack()` clears `isUsbBoot`, the system was trapped in quiescent mode whenever no BLE peer was connected.

## Goals / Non-Goals

**Goals:**
- Guarantee that clicking the power button while docked/charging transitions the system to `ON` (`s_operationalMode = true`).
- Single-point consumption of the button click event in `controlTask()` with explicit delegation to `VehicleController::handlePowerButtonClick()`.
- Ensure `VehicleController::update()` only enforces minimal charging quiescence (stopping audio, sleeping servos, driving charging LED) when `HardwareInit::isUsbBoot()` is active.
- Ensure host unit tests (`test/host_vc`) continue to pass.

**Non-Goals:**
- Changing power latch hardware pins or timing thresholds (`bootLatchS`, `buttonHoldS`).
- Changing charging LED animations or battery cutoff voltage curves.

## Decisions

### Decision 1: Single-Point Button Event Consumption in `controlTask`
- **Choice**: Call `HardwareInit::consumeButtonClicked()` once per 50Hz tick in `controlTask()` in `src/main.cpp`.
- **Rationale**: Having multiple independent callers (`main.cpp` and `VehicleController::update()`) calling a destructive getter (`consumeButtonClicked()`) causes race conditions where one caller starves the other.
- **Implementation**:
  ```cpp
  bool clicked = HardwareInit::consumeButtonClicked();
  if (clicked) {
      if (!s_operationalMode) {
          Serial.println("[MAIN] Power button clicked in charging mode -> Switching ON");
          startOperationalStack();
      }
      VehicleController::handlePowerButtonClick();
  }
  ```
  `VehicleController::handlePowerButtonClick()` will reset `s_disconnectStart` and clear `s_inWarningPhase`. Redundant calls to `consumeButtonClicked()` inside `VehicleController::update()` will be removed.

### Decision 2: Condition Quiescent Charging Mode on `HardwareInit::isUsbBoot()`
- **Choice**: Update the quiescent charging gate in `VehicleController::update()` to:
  ```cpp
  if (s_chargingState != ChargingState::NOT_CHARGING && HardwareInit::isUsbBoot() && !isConnected)
  ```
- **Rationale**: When the vehicle is switched ON, `startOperationalStack()` calls `HardwareInit::clearUsbBoot()`. Once `isUsbBoot` is false, the vehicle is in operational mode; it should advertise BLE, synthesize audio, and follow normal driving/disconnect logic rather than being locked in quiescent charging mode just because the USB cable is attached.
- **Alternatives Considered**: Creating a separate boolean flag in `VehicleController`. Rejected because `HardwareInit::isUsbBoot()` already precisely represents the USB-booted quiescent state and is cleared when switched ON.

## Risks / Trade-offs

- **[Risk] Host test harnesses (`test/host_vc`) simulate GPIOs without `HardwareInit::isUsbBoot()` set.**
  - **Mitigation**: Ensure `host_vc_driver` initializes or simulates `s_isUsbBoot` appropriately for charging test cases, or ensure test assertions align with `isUsbBoot`.
