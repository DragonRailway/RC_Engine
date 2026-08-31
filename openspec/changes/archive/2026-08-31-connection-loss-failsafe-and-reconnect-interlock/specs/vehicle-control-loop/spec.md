## ADDED Requirements

### Requirement: Connection loss failsafe and multi-stage shutdown
When `RadioKit.isConnected()` becomes false, the vehicle controller SHALL execute an immediate safety failsafe:
1. **Drivetrain Deceleration**: Apply 50% braking torque across all drive motors until stopped, then shift gear to Park (P).
2. **Servo & Actuator Depowering**: Detach all steering servos (`HardwareInit::detachServos()`), stop all auxiliary motors, and switch off the auxiliary pump output (`PUMP_ENABLE = LOW`).
3. **Sound Engine Idle Drop**: Immediately drop simulated engine target RPM to idle (0% throttle).
4. **Hazard Warning Lighting**: Automatically activate 4-way Hazard flashing lights to visually indicate loss of control signal.
5. **30-Second Engine Stop**: If disconnection persists for 30 consecutive seconds, the sound engine SHALL trigger a normal engine stop shutdown sequence (`s_engine->stopEngine()`), transitioning engine state to `OFF`.

#### Scenario: Signal loss during driving
- **WHEN** the vehicle is driving at speed and `RadioKit.isConnected()` becomes false
- **THEN** 50% braking is applied, steering servos are detached, aux motors and pump are disabled, engine sound drops to idle, and 4-way hazard lights flash

#### Scenario: Sustained signal loss triggers engine stop
- **WHEN** the vehicle remains disconnected for 30 seconds
- **THEN** the sound engine triggers an engine stop shutdown sequence and transitions to OFF state

### Requirement: Reconnection throttle-to-neutral safety interlock
When `RadioKit.isConnected()` transitions from false to true, the vehicle controller SHALL execute a safety interlock:
1. **Actuator Re-engagement**: Steering servos SHALL re-attach to the commanded position.
2. **Hazard De-escalation**: Automatic failsafe Hazard lighting SHALL deactivate.
3. **Throttle Interlock Latch**: The vehicle controller SHALL engage a safety interlock latching drive torque to zero until the commanded throttle input (`gas_pedal` or `throttle_slider`) returns to neutral / zero (<= 0).

#### Scenario: Controller reconnects with throttle engaged
- **WHEN** the app reconnects while `gas_pedal` or `throttle_slider` is held above zero (> 0)
- **THEN** drivetrain motor output remains locked at zero torque until the user returns throttle to neutral

#### Scenario: Reconnection interlock release
- **WHEN** the reconnected controller throttle returns to neutral (0)
- **THEN** the interlock is released and normal throttle control resumes
