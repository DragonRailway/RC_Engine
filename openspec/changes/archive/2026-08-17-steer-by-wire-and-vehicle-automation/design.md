## Context

The vehicle control loop (`VehicleController.h`) runs at 50-100Hz on the ESP32-S3, mapping inputs received from RadioKit BLE into physical actuator drivers (EasyMotor, EasyServo, EasyLED) and RcEngineSound. 

During physical testing on the MIKRO_V2 board with the Android app, three behavioral disconnects were identified:
1. Steering servo was unpowered/unresponsive when the engine was OFF or in Park (`P`), because `HardwareInit::setServo(steerVal)` was trapped inside `if (eState == RcEngineSound::RUNNING)`.
2. Turn indicators did not auto-cancel on steering return or opposite-steer, unlike real automotive turn signal stalks.
3. Starting and stopping the engine left transmission gear in whatever state was previously selected, requiring manual shifts into Park on shutdown and into Drive on startup.

## Goals / Non-Goals

**Goals:**
- Decouple Ackermann steering servo positioning from engine running state so steer-by-wire responds immediately on accessory power.
- Implement an automotive-grade turn signal cancellation state machine with turn-arm and return thresholds plus opposite-steer cancel.
- Interlock engine start/stop with automatic gear shifting (`OFF -> P`, `START -> D` if currently `P`).
- Ensure all automated UI state updates (`left_indicator`, `right_indicator`, `gear_switch`) synchronize bidirectionally back to the RadioKit app interface.

**Non-Goals:**
- Skid-steer drivetrain track differential mixing when the engine is OFF (skid tracks require motor drive and remain stopped when engine is OFF).
- Modifying RadioKit layout or widget coordinates in `radiokit-rc-ui-design.json`.

## Decisions

### 1. Direct Steer-by-Wire Feed
- **Decision**: In `VehicleController::update()`, call `HardwareInit::setServo(steerVal)` continuously for Ackermann steering, independent of `eState`.
- **Rationale**: Steering servos consume minimal quiescent current and should follow the steering wheel angle continuously in steer-by-wire systems.
- **Alternatives Considered**: Gating on `eState != OFF`. Rejected because `gear == P` puts `eState` into `PARKING_BRAKE`, which would still freeze steering while parked.

### 2. Turn Stalk Cancelation State Machine
- **Decision**: Track `s_leftTurnArmed` and `s_rightTurnArmed`.
  - If `left_indicator` is ON:
    - Arm left turn when `steerVal < -20`.
    - If armed and `steerVal > -8`, cancel left indicator (`left_indicator.rk.state = false; s_leftTurnArmed = false;`).
    - If `steerVal > +15` (opposite steer), cancel left indicator immediately.
  - Symmetrical logic for `right_indicator`.
  - Mutual exclusion: engaging Left clears Right and vice-versa.
- **Rationale**: Matches standard automotive stalk kinematics with a 12% hysteresis band (-20% to -8%) preventing false cancellation during slight steering adjustments.

### 3. Engine Start/Stop Gear Interlock
- **Decision**:
  - Detect edge of `engineStartToggle` (from `start_button.rk.state` on TRUCK):
  - Rising edge (engine start): If `gear == 1` (P), set `gear_switch.rk.value = 0` (D).
  - Falling edge (engine stop) or when `eState == OFF`: Force `gear_switch.rk.value = 1` (P).
- **Rationale**: Mirrors modern start/stop pushbutton transmissions (e.g. auto-park on shutdown, auto-drive on start).

## Risks / Trade-offs

- **[Risk]** Rapid steering oscillations cancelling turn signals prematurely.
  - **Mitigation**: 12% hysteresis window (`< -20%` to arm, `> -8%` to trigger) ensures cancellation only occurs when the wheel returns towards neutral or crosses zero.
- **[Risk]** Out-of-sync widget state between board and app.
  - **Mitigation**: RadioKit bidirectional widgets propagate firmware state changes (`rk.state`, `rk.value`) to the app on the next BLE notification frame.
