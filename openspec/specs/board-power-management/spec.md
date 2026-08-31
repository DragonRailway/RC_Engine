# board-power-management Specification

## Purpose
TBD - created by archiving change board-power-management. Update Purpose after archive.
## Requirements
### Requirement: Configurable power time parameters in seconds

The hardware configuration schema (`configs/schemas/hardware_config.schema.json`) SHALL support a `"power"` configuration object containing timing values in seconds:
- `boot_latch_s`: float/number, default `1.0` seconds (range `0.0`–`30.0`s)
- `button_hold_s`: float/number, default `4.0` seconds (range `1.0`–`30.0`s)
- `disconnect_timeout_s`: float/number, default `60.0` seconds (range `0.0`–`3600.0`s; 0 disables auto-off)
- `warning_window_s`: float/number, default `10.0` seconds (range `1.0`–`60.0`s)
- `cutoff_delay_s`: float/number, default `1.5` seconds (range `0.0`–`60.0`s)

#### Scenario: Custom power timing values in hardware config
- **WHEN** hardware config specifies `"power": { "boot_latch_s": 2.0, "button_hold_s": 5.0, "disconnect_timeout_s": 120.0, "warning_window_s": 15.0, "cutoff_delay_s": 2.0 }`
- **THEN** `ConfigParser` populates `HardwareConfig::power` with those exact seconds values

---

### Requirement: 3-State board power management (OFF, ON, CHARGING)

The firmware SHALL track board state as `OFF`, `ON`, or `CHARGING` (detected via `POWER::CHARGE_SENS` input pin).
When `CHARGE_SENS` is active:
1. Board state enters `CHARGING`.
2. All motor outputs are set to zero/disabled.
3. Disconnect auto power-off is suspended.

When `CHARGE_SENS` transitions to inactive:
1. Board state returns to `ON` (or `OFF` if unlatched).

#### Scenario: Charger plugged in during operation
- **WHEN** `CHARGE_SENS` pin reads active HIGH while running
- **THEN** motor drive is stopped, disconnect auto power-off timer is suspended, and board enters `CHARGING` state

---

### Requirement: Disconnect auto power-off with warning window and button reset

When `RadioKit.isConnected()` is false, the system SHALL run a disconnection timer up to `disconnect_timeout_s`.
1. **Warning Phase**: During the final `warning_window_s` seconds before timeout, the system SHALL trigger visual hazard warning blinks and audio alerts.
2. **Power Button Reset**: If the physical power button is pressed/clicked during disconnection or during the warning phase, the system SHALL reset the disconnection timer to 0, canceling the warning state.
3. **App Reconnection**: Re-establishing a transport connection SHALL cancel the warning and reset the timer.
4. **Auto Power-Off**: If the timeout elapses without button press or app reconnection, `HardwareInit::powerOff()` SHALL execute.

#### Scenario: Disconnect timeout warning and power button timer reset
- **WHEN** vehicle remains disconnected for `disconnect_timeout_s - warning_window_s`
- **THEN** warning lights and audio alert activate
- **WHEN** physical power button is pressed/clicked during the warning phase
- **THEN** disconnection timer resets to 0 and warning phase cancels

### Requirement: Failsafe coordination with power timeout
The disconnect failsafe phases SHALL coordinate cleanly with the board's `disconnect_timeout_s` timer:
1. Short disconnects (< 30s) preserve engine idle and allow instant throttle recovery upon zero-crossing.
2. Medium disconnects (30s to `disconnect_timeout_s`) shut down engine audio and preserve low power state.
3. Sustained disconnects exceeding `disconnect_timeout_s` trigger complete board shutdown via `HardwareInit::powerOff()`.

#### Scenario: Full disconnect timeout progression
- **WHEN** signal is lost and not restored
- **THEN** vehicle brakes to stop, detaches servos, drops sound to idle at t=0, stops engine sound at t=30s, and powers down board at t=disconnect_timeout_s

