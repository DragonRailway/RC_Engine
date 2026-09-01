## Context

In ESP-IDF v5.x, the MCPWM peripheral drivers (`mcpwm_timer`, `mcpwm_oper`, `mcpwm_cmpr`, `mcpwm_gen`) are tied to hardware allocations. When `EasyServo::detach()` is called, these driver objects are deleted and unallocated. Calling `EasyServo::attach()` allocates new MCPWM driver handles.

In firmware execution, failsafe triggers when wireless connection drops (`!RadioKit.isConnected()`), requiring steering and auxiliary servos to depower immediately to prevent stall burnouts if stalled against obstacles. When reconnected, servos re-engage. Additionally, during `HardwareInit::stopAll()` (e.g. shutdown / power off), all actuator outputs are stopped.

Using `detach()` during these frequent runtime transitions causes ESP-IDF MCPWM driver handle churn, memory/ISR exhaustion risks, and re-allocation latency. In contrast, `EasyServo::sleep()` forces the generator output level to LOW (`mcpwm_generator_set_force_level(gen, 0, true)`) while retaining the driver structure in hardware, and `EasyServo::wake()` immediately unfreezes the generator (`mcpwm_generator_set_force_level(gen, -1, true)`) and updates duty.

## Goals / Non-Goals

**Goals:**
- Replace `HardwareInit::detachServos()` and `HardwareInit::attachServos()` with `HardwareInit::sleepServos()` and `HardwareInit::wakeServos()`.
- Sleep all steering servos and auxiliary servos (any aux output configured as a servo/ESC) on disconnect failsafe.
- Wake all steering servos and auxiliary servos on connection restore.
- Use `sleep()` instead of `detach()` in `HardwareInit::stopAll()` for steering, aux, and ESC servos.
- Ensure host tests (`test/host_vc/`) validate the sleep/wake lifecycle.

**Non-Goals:**
- Modifying `ESP32_EasyKit` library (vendored EasyKit already has `sleep()` and `wake()`).
- Changing motor driver PWM or LED channel lifecycle.

## Decisions

1. **Explicit `sleepServos()` and `wakeServos()` API**:
   - *Decision*: Introduce `HardwareInit::sleepServos()` and `HardwareInit::wakeServos()`, replacing `detachServos()` / `attachServos()`.
   - *Rationale*: Accurately reflects the hardware state (MCPWM generators forced low, timers preserved) and eliminates ambiguity between hardware destruction (`detach`) and PWM pulse suppression (`sleep`).

2. **Broad Servo Coverage (Steering + Aux Servos)**:
   - *Decision*: `sleepServos()` will iterate and sleep all active steering servos (`steeringServos[0..s_steeringServoCount-1]`) and aux servos (`auxServos[0..MAX_AUX_MOTORS-1]`). `wakeServos()` will wake them and restore their commanded positions.
   - *Rationale*: Any auxiliary servo (e.g., dump bed, crane, pan/tilt) should also be unpowered during signal loss to prevent motor stall burnout.

3. **`stopAll()` Teardown using `sleep()`**:
   - *Decision*: In `HardwareInit::stopAll()`, call `.stop()` and `.sleep()` on all steering, aux, and ESC servos instead of `.detach()`.
   - *Rationale*: Leaves output pins pulled low before ESP-IDF deep sleep without deleting MCPWM driver handles or triggering driver bugs on shutdown.

## Risks / Trade-offs

- **[Risk] Servo limpness during signal loss** → Handled: Servos with external mechanical load may drift when PWM pulses are cut. This is standard RC failsafe behavior to prevent servo gear stripping and stall current burnout.
- **[Risk] Host stubs out of sync** → Mitigation: Update `test/host_vc/host_easykit_stubs.cpp` to implement `sleep()` and `wake()` on `EasyServo` stub class, and assert `host_servo_sleeping` in `host_vc_driver.cpp`.
