## MODIFIED Requirements

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
   - The system SHALL execute `HardwareInit::powerOff()` to safely power down the board.

#### Scenario: Boot into minimal charging mode upon USB connection
- **WHEN** board boots from USB connection with power button unpressed
- **THEN** board latches power, enters `CHARGING_ACTIVE` (or `CHARGING_COMPLETE` if battery is full), leaves BLE modem and audio synthesis disabled, and suspends disconnect timeout

#### Scenario: User switches vehicle ON from charging mode
- **WHEN** user clicks or presses the physical power button while in charging mode
- **THEN** board transitions to `ON` state, starts RadioKit BLE advertising, initializes audio output, and enters normal operational control

#### Scenario: USB unplugged while in charging mode
- **WHEN** USB power is removed while the board remains in charging mode without being switched ON
- **THEN** board shuts down via `HardwareInit::powerOff()`

---

## ADDED Requirements

### Requirement: Halting and deep sleep power off behavior on USB
When `HardwareInit::powerOff()` executes, the system SHALL manage power-off states safely without triggering reset loops:
1. `POWER::POWER_ENABLE` SHALL be set to `LOW` to cut battery power on battery-operated boards.
2. The firmware SHALL NOT arm a 0-microsecond RTC wakeup timer via `esp_deep_sleep(0)`.
3. If USB power persists, the MCU SHALL halt cleanly or enter deep sleep with wake sources configured strictly on the power button input so as not to trigger an automated reboot loop.

#### Scenario: Power off invoked while USB connected
- **WHEN** `HardwareInit::powerOff()` is invoked while USB is plugged in
- **THEN** system stops outputs, unlatches power enable, and halts without immediately rebooting into `setup()`
