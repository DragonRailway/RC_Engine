## Context

In real-world vehicle dynamics, front wheels exhibit self-aligning torque due to positive caster angle and pneumatic trail. This torque naturally pulls steered wheels back to center ($0^\circ$) as vehicle speed increases. Conversely, when stationary, dry friction holds the wheels in position unless acted upon by the driver.

In RC_Engine, the RadioKit steering widget (`steering_wheel` / `RK_Knob`) is typically configured with `RK_SPRING_NONE` for realistic touch control. Without active auto-centering, letting go of the steering wheel leaves the vehicle turning in a continuous circle indefinitely.

## Goals / Non-Goals

**Goals:**
- Provide realistic speed-dependent steering return-to-center physics that scale smoothly with forward motor speed.
- Support both Ackermann (servo) and Skid-Steer (dual-motor differential) drivetrains.
- Distinguish between active user steering and passive/released steering state.
- Keep the RadioKit mobile app visually synchronized by pushing decayed values back to `steering_wheel.rk.value`, causing the on-screen steering wheel to visibly unwind back to straight.
- Allow full tunability under the hardware config `drivetrain.auto_centering` block.

**Non-Goals:**
- Replace or override spring-return configurations defined explicitly in the UI design.
- Force auto-centering while the user is actively touching or adjusting the steering wheel.
- Alter locomotive steering behavior (steering remains locked to 0 for locomotives).

## Decisions

### 1. Configuration in `drivetrain.auto_centering`
We place the configuration inside the `drivetrain` block in `HardwareConfig`:
```json
"drivetrain": {
  "type": "ackermann",
  "auto_centering": {
    "enabled": true,
    "base_rate": 0.0,
    "speed_rate": 1.8,
    "max_rate": 8.0,
    "hold_in_reverse": true
  }
}
```
*Rationale:* Auto-centering is a fundamental drivetrain steering property alongside `steering_servos` and `steering_sensitivity`.

### 2. User Touch / Activity Detection
- The firmware monitors incoming updates to `steering_wheel.rk.value`.
- An internal activity timestamp `s_lastSteerTouchTime` records when values are updated from the app.
- If no user input has occurred for an activity timeout ($T_{active} \approx 120\text{ ms}$), the steering wheel is marked as passive / released.
*Rationale:* RadioKit streams input updates continuously while a user is dragging on the touch screen. Once released, the stream halts, allowing firmware auto-centering to take control without fighting the user.

### 3. Speed-Proportional Decay Function
During each control loop tick ($\Delta t = \text{now} - \text{lastTick}$):
$$\text{Rate} = \text{base\_rate} + \left(\text{speed\_rate} \times \frac{|\text{motorSpeed}|}{100}\right)$$
$$\text{ClampedRate} = \min(\text{Rate}, \text{max\_rate})$$
$$\text{DecayStep} = \text{ClampedRate} \times \left(\frac{\Delta t}{20.0}\right)$$
- If $\text{currentSteer} > 0$: $\text{currentSteer} = \max(0.0, \text{currentSteer} - \text{DecayStep})$
- If $\text{currentSteer} < 0$: $\text{currentSteer} = \min(0.0, \text{currentSteer} + \text{DecayStep})$
- In reverse gear when `hold_in_reverse` is `true`: $\text{Rate} = \text{base\_rate}$ (speed rate disabled, simulating caster trailing dynamics).

### 4. Bidirectional RadioKit UI Synchronization
When $\text{currentSteer}$ decays, the firmware updates `steering_wheel.rk.value = (int8_t)round(currentSteer)`.
`RadioKit.update()` detects the difference from `_shadowInput` and pushes `RK_CMD_SET_INPUT` to the connected app, keeping the UI steering wheel aligned with the physical wheels.

## Risks / Trade-offs

- **[Risk] Touch packet latency / jitter causes momentary auto-centering during slow drags:**
  → *Mitigation:* A 120ms hysteresis timeout window ensures continuous touch streams aren't interrupted by minor packet spacing.
- **[Risk] Rapid BLE packet saturation from high-frequency UI updates:**
  → *Mitigation:* `steering_wheel.rk.value` is an integer (`int8_t`). Updates only dispatch to the app when the rounded integer value changes, naturally rate-limiting UI push frames.
