# vehicle-control-loop Specification

## Purpose
Main vehicle drive loop, steering calculations, lighting mapping, and controller telemetry.

## MODIFIED Requirements

### Requirement: Connection loss failsafe and multi-stage shutdown
When `RadioKit.isConnected()` becomes false, the vehicle controller SHALL execute an immediate safety failsafe:
1. **Drivetrain Deceleration**: Feed 50% braking command to `EngineSim` until stopped, then shift gear to Park (P).
2. **Servo & Actuator Depowering**: Put all steering servos and auxiliary servos to sleep (`HardwareInit::sleepServos()`), stop all auxiliary motors, and switch off the auxiliary pump output (`PUMP_ENABLE = LOW`).
3. **Sound Engine Idle Drop**: Pass zero throttle command to `EngineSim` dropping engine target RPM to idle.
4. **Hazard Warning Lighting**: Automatically activate 4-way Hazard flashing lights to visually indicate loss of control signal.
5. **30-Second Engine Stop**: If disconnection persists for 30 consecutive seconds, `EngineSim` SHALL trigger a normal engine stop shutdown sequence (`s_engineSim->stopEngine()`), transitioning engine state to `OFF`.

#### Scenario: Signal loss during driving
- **WHEN** the vehicle is driving at speed and `RadioKit.isConnected()` becomes false
- **THEN** 50% braking is fed to `EngineSim`, steering and auxiliary servos are put to sleep, aux motors and pump are disabled, engine sound drops to idle, and 4-way hazard lights flash

#### Scenario: Sustained signal loss triggers engine stop
- **WHEN** the vehicle remains disconnected for 30 seconds
- **THEN** `EngineSim` triggers an engine stop shutdown sequence and transitions to OFF state

## ADDED Requirements

### Requirement: Decoupled hardware I/O dispatch
The `VehicleController` SHALL delegate all powertrain physics, motor speed ramping, and transmission slip calculations to `EngineSim`, directly passing the computed motor speed (`s_engineSim->getMotorSpeed()`) to `HardwareInit::setMotor` or `HardwareInit::setSkidMotors`.

#### Scenario: Periodic control dispatch
- **WHEN** `VehicleController::update()` executes
- **THEN** inputs are passed to `EngineSim`, `SoundSynth` is synchronized with `EngineSim`, and physical actuator PWM is updated directly from simulation outputs
