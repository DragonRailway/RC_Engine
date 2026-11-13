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

The firmware SHALL track board operational state as `OFF`, `ON`, `CHARGING_ACTIVE`, or `CHARGING_COMPLETE`.
1. **Docked / USB Detection**: The board is considered docked/plugged when `POWER::CHARGE_SENS` is active (`LOW`), USB JTAG/CDC is connected, or battery voltage sense reflects external USB power ($V_{\text{bat}} > 4.35\text{V}$ for 1S).
2. **Minimal Charging Operation**: While in `CHARGING_ACTIVE` or `CHARGING_COMPLETE`:
   - Radio modem (BLE advertising and RadioKit stack) SHALL remain disabled.
   - Audio synthesis and I2S DAC output SHALL remain stopped and muted.
   - Motor drivers SHALL be stopped and steering/aux servos SHALL be placed in sleep mode.
   - Disconnect auto power-off timers SHALL remain suspended.
3. **Power Button Exit ("Switch On")**: When the physical power button is pressed/clicked while in `CHARGING_ACTIVE` or `CHARGING_COMPLETE`:
   - The board SHALL transition to `ON`.
   - The RadioKit BLE stack and audio synthesis engine SHALL initialize and start.
4. **Unplug Power-Off**: If the vehicle is disconnected from USB power while in `CHARGING_ACTIVE` or `CHARGING_COMPLETE` without having been switched `ON`:
   - The system SHALL blink the charging indicator (or headlight / indicator pin fallback) 4 times quickly (200ms cycle: 100ms ON / 100ms OFF) while keeping `POWER_ENABLE` latched.
   - Immediately following the 4th blink, the system SHALL execute `HardwareInit::powerOff()` to release `POWER_ENABLE` and safely power down the board.

#### Scenario: Boot into minimal charging mode upon USB connection
- **WHEN** board boots from USB connection with power button unpressed
- **THEN** board latches power, enters `CHARGING_ACTIVE` (or `CHARGING_COMPLETE` if battery is full), leaves BLE modem and audio synthesis disabled, and suspends disconnect timeout

#### Scenario: User switches vehicle ON from charging mode
- **WHEN** user clicks or presses the physical power button while in charging mode
- **THEN** board transitions to `ON` state, starts RadioKit BLE advertising, initializes audio output, and enters normal operational control

#### Scenario: USB unplugged while in charging mode
- **WHEN** USB power is removed while the board remains in charging mode without being switched ON
- **THEN** board blinks the charging indicator 4 times quickly (200ms cycle) and then shuts down via `HardwareInit::powerOff()`

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
- **THEN** vehicle brakes to stop, puts servos to sleep, drops sound to idle at t=0, stops engine sound at t=30s, and powers down board at t=disconnect_timeout_s

---

### Requirement: Halting and deep sleep power off behavior on USB
When `HardwareInit::powerOff()` executes, the system SHALL manage power-off states safely without triggering reset loops:
1. `POWER::POWER_ENABLE` SHALL be set to `LOW` to cut battery power on battery-operated boards.
2. The firmware SHALL NOT arm a 0-microsecond RTC wakeup timer via `esp_deep_sleep(0)`.
3. If USB power persists, the MCU SHALL halt cleanly or enter deep sleep with wake sources configured strictly on the power button input so as not to trigger an automated reboot loop.

#### Scenario: Power off invoked while USB connected
- **WHEN** `HardwareInit::powerOff()` is invoked while USB is plugged in
- **THEN** system stops outputs, unlatches power enable, and halts without immediately rebooting into `setup()`

