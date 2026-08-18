## Why

Physical testing on the MIKRO_V2 board revealed three functional issues in the vehicle control loop:
1. Steering servo output was gated inside `eState == RUNNING`. Because the board boots with engine `OFF`, and shifting to Park `P` enters `eState == PARKING_BRAKE`, the steering servo was completely unresponsive in Park and with engine stopped. In steer-by-wire RC systems, steering servo angle must track the steering wheel knob continuously on accessory power regardless of engine running state.
2. Turn indicators did not behave like real vehicle stalks. While indicator buttons are toggle widgets, real vehicles auto-cancel turn signals when the steering wheel returns from a turn towards center or steers in the opposite direction.
3. Starting and stopping the engine required manual separate gear shifts. Real automatic transmissions and intuitive RC vehicle operation should automatically shift into Park (`P`) when the engine is stopped, and auto-shift to Drive (`D`) when the engine is started.

## What Changes

- **Continuous Steer-by-Wire Servo Control**: Decouple `HardwareInit::setServo(steerVal)` from `eState == RUNNING`. In Ackermann steering setups, steering servo follows `steerVal` continuously during all active states (excluding low-voltage battery cutoff).
- **Real-Vehicle Turn Indicator Cancelation with Hysteresis**:
  - Toggling Left/Right indicator button engages the corresponding turn signal.
  - When Left indicator is active and the vehicle steers Left past threshold (`steerVal < -20°`), the turn is armed. When steering angle returns towards center (`steerVal > -8°`), the indicator auto-cancels (`left_indicator.rk.state = false`), updating the UI toggle button.
  - If Left indicator is active and the driver steers Right (`steerVal > +15°`), the indicator auto-cancels immediately.
  - Symmetrical auto-cancelation applies for the Right indicator.
- **Engine Start/Stop Transmission Interlock**:
  - On engine stop (`start_button` OFF / engine stopped), transmission automatically shifts to Park `P` (`gear_switch.rk.value = 1`), locking motor output and updating the UI switch.
  - On engine start (`start_button` ON / engine start), if currently in `P`, transmission automatically shifts to Drive `D` (`gear_switch.rk.value = 0`), unlocking motor output and updating the UI switch.

## Capabilities

### Modified Capabilities
- `vehicle-control-loop`: Update steering servo execution flow, turn indicator auto-cancel state machine, and engine start/stop gear interlock.
- `advanced-lighting-automation`: Update turn signal stalk cancelation semantics and UI state synchronization.

## Impact
- `common/VehicleController.h`: Update loop execution logic, turn signal state machine, and gear transition handlers.
- `test/test_host_vc/`: Update host test harness and test assertions for steering in Park/OFF, turn cancelation, and gear transitions.
