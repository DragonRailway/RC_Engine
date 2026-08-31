## Context

The RC_Engine firmware runs on ESP32-S3 boards controlled over BLE/Wi-Fi using the RadioKit protocol. When the control connection drops, the vehicle must execute a structured, fail-safe shutdown sequence rather than retaining the last received control inputs in memory.

## Goals / Non-Goals

**Goals:**
- **Controlled Deceleration**: Decelerate all vehicles using 50% braking stop to avoid mechanical gear shock and runaway motion.
- **Actuator Depower**: Detach steering servos and switch off auxiliary motors and hydraulic pump outputs.
- **Lighting & Audio Sequence**: Drop simulated RPM to idle, turn on 4-way hazard blinkers, shut down sound engine after 30 seconds of disconnect, and power off the board at `disconnect_timeout_s`.
- **Reconnection Interlock**: Require user throttle to be at zero/neutral before unblocking drive torque when the controller reconnects.

**Non-Goals:**
- Modifying RadioKit BLE reconnect transport internals.
- Dynamic steering gyro stabilization during signal loss.

## Decisions

### 1. 50% Braking Stop vs Instant 100% Lock vs Coasting
- **Decision**: Apply 50% moderate braking torque during signal loss, shifting to Park (P) once stopped.
- **Rationale**: 100% immediate lock can strip small plastic gears or derail locomotives, while pure coasting can result in lengthy runaway rollouts down gradients. 50% braking provides controlled deceleration.

### 2. Servo Detach vs Centering Hold
- **Decision**: Detach all steering servos immediately (`HardwareInit::detachServos()`).
- **Rationale**: If the vehicle stops against an obstacle or kerb, holding a fixed angle forces continuous stall current through the servo motor and FETs, risking burnout. Detaching disables PWM pulses, allowing the servo to rest unpowered. On reconnect, servos re-attach smoothly to the commanded position.

### 3. Progressive Audio & Power Shutdown
- **Decision**:
  - $t = 0$: Engine RPM drops to idle ($0\%$ throttle target); Hazard 4-way lights turn ON.
  - $t = 30\text{s}$: Engine sound triggers shutdown sequence (`s_engine->stopEngine()`), transitioning state to `OFF`.
  - $t = \text{disconnect\_timeout\_s}$: Full power-latch shutdown (`HardwareInit::powerOff()`).
- **Rationale**: Keeps engine sound responsive during brief transient dropouts (1–10s) without jarring stop/re-starts, but cleanly turns off audio if signal loss is sustained.

### 4. Reconnection Throttle Interlock
- **Decision**: Latch `s_reconnectThrottleInterlock = true` when `RadioKit.isConnected()` transitions from `false` to `true`.
- **Rationale**: If the controller reconnects while the user's thumb is still on the gas pedal or slider, the vehicle must NOT suddenly lurch forward. Motor torque remains 0 until the throttle reading passes through zero (`throttlePct <= 0` or pedal centered).

## Risks / Trade-offs

- [Risk] Reconnection interlock might confuse a user if they don't know why throttle isn't responding immediately upon reconnect.
  - *Mitigation*: The interlock clears automatically the moment the user relaxes the joystick/slider back to neutral (0).
- [Risk] Re-attaching servos upon reconnect could cause a sudden snap if steering slider is off-center.
  - *Mitigation*: EasyServo re-attaches cleanly at the currently received angle, and steering auto-centering will smoothly handle centering.
