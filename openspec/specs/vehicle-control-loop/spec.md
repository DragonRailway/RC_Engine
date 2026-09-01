# vehicle-control-loop Specification

## Purpose
Main vehicle drive loop, steering calculations, lighting mapping, and controller telemetry.
## Requirements
### Requirement: Light control
The firmware SHALL map the `truck_light` (Truck page) and `loco_light` (Loco page) multi-select bitmasks, as well as automatic steering indicators, dynamic deceleration brake lights, and low-battery hazard overrides, to the configured light outputs (head, tail, brake, turn, reversing). The firmware SHALL compute an 8-bit configured light mask (`HardwareInit::getConfiguredLightMask`) that enables visibility exclusively for channels configured in the hardware config:
- For Trucks: Bit 0 = Head Light, Bit 1 = High Beam (enabled if `full_beam` or `head_light` configured), Bit 2 = Fog Lamp, Bit 3 = Turn / Hazard, Bit 4 = Beacon, Bit 5 = Cab Light, Bit 6 = Work Light, Bit 7 = Aux Light.
- For Locomotives: Bit 0 = Directional Headlight / Tail, Bit 1 = Fog Lamp, Bit 2 = Ditch Lights, Bit 3 = Beacon, Bit 4 = Cab Light, Bit 5 = Step Light, Bit 6 = Aux / Work Light, Bit 7 = Hazard / Warning.

#### Scenario: LED group selected
- **WHEN** an item is selected on `truck_light` or `loco_light`
- **THEN** the corresponding configured light outputs are enabled via PWM

#### Scenario: All lights deselected
- **WHEN** no items are selected on a light widget and no automation is active
- **THEN** the corresponding light outputs are disabled

#### Scenario: Configured light mask visibility filtering
- **WHEN** the board initializes or hot-reloads hardware configuration
- **THEN** `truck_light` and `loco_light` widgets are updated with item masks enabling only the buttons corresponding to configured hardware channels

### Requirement: Connection loss failsafe and multi-stage shutdown
When `RadioKit.isConnected()` becomes false, the vehicle controller SHALL execute an immediate safety failsafe:
1. **Drivetrain Deceleration**: Apply 50% braking torque across all drive motors until stopped, then shift gear to Park (P).
2. **Servo & Actuator Depowering**: Put all steering servos and auxiliary servos to sleep (`HardwareInit::sleepServos()`), stop all auxiliary motors, and switch off the auxiliary pump output (`PUMP_ENABLE = LOW`).
3. **Sound Engine Idle Drop**: Immediately drop simulated engine target RPM to idle (0% throttle).
4. **Hazard Warning Lighting**: Automatically activate 4-way Hazard flashing lights to visually indicate loss of control signal.
5. **30-Second Engine Stop**: If disconnection persists for 30 consecutive seconds, the sound engine SHALL trigger a normal engine stop shutdown sequence (`s_engine->stopEngine()`), transitioning engine state to `OFF`.

#### Scenario: Signal loss during driving
- **WHEN** the vehicle is driving at speed and `RadioKit.isConnected()` becomes false
- **THEN** 50% braking is applied, steering and auxiliary servos are put to sleep, aux motors and pump are disabled, engine sound drops to idle, and 4-way hazard lights flash

#### Scenario: Sustained signal loss triggers engine stop
- **WHEN** the vehicle remains disconnected for 30 seconds
- **THEN** the sound engine triggers an engine stop shutdown sequence and transitions to OFF state

### Requirement: Reconnection throttle-to-neutral safety interlock
When `RadioKit.isConnected()` transitions from false to true, the vehicle controller SHALL execute a safety interlock:
1. **Actuator Re-engagement**: Steering servos and auxiliary servos SHALL wake (`HardwareInit::wakeServos()`) and restore commanded positions.
2. **Hazard De-escalation**: Automatic failsafe Hazard lighting SHALL deactivate.
3. **Throttle Interlock Latch**: The vehicle controller SHALL engage a safety interlock latching drive torque to zero until the commanded throttle input (`gas_pedal` or `throttle_slider`) returns to neutral / zero (<= 0).

#### Scenario: Controller reconnects with throttle engaged
- **WHEN** the app reconnects while `gas_pedal` or `throttle_slider` is held above zero (> 0)
- **THEN** drivetrain motor output remains locked at zero torque until the user returns throttle to neutral

#### Scenario: Reconnection interlock release
- **WHEN** the reconnected controller throttle returns to neutral (0)
- **THEN** the interlock is released and normal throttle control resumes

### Requirement: Deterministic 50 Hz Periodic Control Task
The firmware SHALL execute the main vehicle control loop (`VehicleController::update()`) and peripheral animation update (`HardwareInit::update()`) inside a dedicated periodic FreeRTOS task running at a strict 50 Hz (20.0 ms) period:
1. The task SHALL use `vTaskDelayUntil()` to eliminate timing jitter caused by BLE packet processing and serial telemetry.
2. The task SHALL be pinned to Core 1 at priority 2, running below the real-time audio task (priority 3) and above the base Arduino `loopTask` (priority 1).

#### Scenario: Steady 50 Hz loop period during high BLE traffic
- **WHEN** the BLE transport receives high-bandwidth telemetry notifications or control packets
- **THEN** `VehicleController::update()` executes at steady 20.0 ms intervals without timing jitter or frame drops

### Requirement: Event-driven configuration reload
The firmware SHALL reload configuration files upon receiving an explicit filesystem upload completion event from RadioKit, eliminating periodic filesystem `stat()` polling:
1. The main loop SHALL NOT execute periodic `stat()` or `fileWriteTime()` checks against LittleFS during normal operation.
2. When a file upload completes, the reload handler SHALL execute safely to hot-reload hardware and vehicle configs.

#### Scenario: File upload triggers instant reload
- **WHEN** a new `vehicle.json` or `hardware.json` is uploaded over RadioKit
- **THEN** the firmware reloads the configurations without requiring periodic timer polling

### Requirement: Decimated 5 Hz battery voltage ADC sampling
The vehicle control loop SHALL decimate battery voltage ADC acquisition from 50 Hz to 5 Hz (every 10 control ticks / 200 ms):
1. `HardwareInit::readBatteryVoltage()` SHALL be invoked once every 10 control loop iterations (200 ms).
2. The sampled voltage SHALL be smoothed using an exponential moving average (EMA) low-pass filter ($\alpha = 0.1$).
3. Battery warning and cutoff thresholds SHALL be evaluated against the smoothed voltage.

#### Scenario: High motor acceleration load
- **WHEN** full throttle causes high-current motor switching noise on the power rails
- **THEN** battery voltage sampling filters out transient switching ripple and avoids false low-voltage cutoffs

### Requirement: Synchronous Turn and Hazard Signal Control
The firmware SHALL control turn indicators and hazard warning lights through a unified synchronized interface (`HardwareInit::setTurnSignals`) ensuring:
1. When Hazard mode is active, both left and right turn signal LEDs flash in perfect synchronous phase (0 deg phase offset).
2. When transitioning from a single turn indicator (Left or Right) to Hazard mode, both blink timers SHALL be reset simultaneously in the same cycle.
3. The dashboard indicator audio click sound SHALL be synchronized with the active blink phase.

#### Scenario: Hazard lights activated while turn indicator is blinking
- **WHEN** the left turn signal is blinking and hazard warning is turned ON
- **THEN** both left and right indicators SHALL immediately synchronize and flash together in lockstep without phase offset.

#### Scenario: Hazard lights deactivated with no turn indicator
- **WHEN** hazard warning is turned OFF and no turn signals are active
- **THEN** both turn signal LEDs SHALL stop blinking and remain OFF.

#### Scenario: Turn indicator active without hazard
- **WHEN** left turn indicator is active and hazard is OFF
- **THEN** only the left turn LED SHALL blink while the right LED remains OFF.


