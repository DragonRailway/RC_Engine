## MODIFIED Requirements

### Requirement: 3-State board power management (OFF, ON, CHARGING)

The firmware SHALL track board operational state as `OFF`, `ON`, `CHARGING_ACTIVE`, or `CHARGING_COMPLETE`.
1. **Docked / USB Detection**: The board is considered docked/plugged when `POWER::CHARGE_SENS` is active (`LOW`), USB JTAG/CDC is connected, or battery voltage sense reflects external USB power ($V_{\text{bat}} > 4.35\text{V}$ for 1S).
2. **Minimal Charging Operation**: While in `CHARGING_ACTIVE` or `CHARGING_COMPLETE` prior to being switched ON:
   - Radio modem (BLE advertising and RadioKit stack) SHALL remain disabled.
   - Audio synthesis and I2S DAC output SHALL remain stopped and muted.
   - Motor drivers SHALL be stopped and steering/aux servos SHALL be placed in sleep mode.
   - Disconnect auto power-off timers SHALL remain suspended.
3. **Power Button Exit ("Switch On")**: When the physical power button is pressed/clicked while in `CHARGING_ACTIVE` or `CHARGING_COMPLETE`:
   - The button click event SHALL NOT be dropped or double-consumed by internal subsystems.
   - The board SHALL transition to `ON`.
   - The RadioKit BLE stack and audio synthesis engine SHALL initialize and start.
   - While in `ON` state with USB connected, the system SHALL remain operational (advertising BLE, running audio, and responsive to connection) and SHALL NOT be forced back into quiescent charging mode.
4. **Unplug Power-Off**: If the vehicle is disconnected from USB power while in `CHARGING_ACTIVE` or `CHARGING_COMPLETE` without having been switched `ON`:
   - The system SHALL execute `HardwareInit::powerOff()` to safely power down the board.

#### Scenario: Boot into minimal charging mode upon USB connection
- **WHEN** board boots from USB connection with power button unpressed
- **THEN** board latches power, enters `CHARGING_ACTIVE` (or `CHARGING_COMPLETE` if battery is full), leaves BLE modem and audio synthesis disabled, and suspends disconnect timeout

#### Scenario: User switches vehicle ON from charging mode
- **WHEN** user clicks or presses the physical power button while in charging mode
- **THEN** button click is captured, board transitions to `ON` state, starts RadioKit BLE advertising, initializes audio output, and remains in operational state even if USB is still connected

#### Scenario: USB unplugged while in charging mode
- **WHEN** USB power is removed while the board remains in charging mode without being switched ON
- **THEN** board shuts down via `HardwareInit::powerOff()`
