## MODIFIED Requirements

### Requirement: 3-State board power management (OFF, ON, CHARGING)

The firmware SHALL track board operational state as `OFF`, `ON`, `CHARGING_ACTIVE`, or `CHARGING_COMPLETE`.
1. **Docked / USB Detection**: The board is considered docked/plugged when `POWER::CHARGE_SENS` is active (`LOW`), USB JTAG/CDC is connected, or battery voltage sense reflects external USB power ($V_{\text{bat}} > 4.35\text{V}$ for 1S).
2. **Minimal Charging Operation**: While in `CHARGING_ACTIVE` or `CHARGING_COMPLETE`:
   - Radio modem (BLE advertising and RadioKit stack) SHALL remain disabled.
   - Audio synthesis and I2S DAC output SHALL remain stopped and muted.
   - Motor drivers SHALL be stopped and steering/aux servos SHALL be placed in sleep mode.
   - Disconnect auto power-off timers SHALL remain suspended.
   - `POWER_ENABLE` SHALL remain latched ON to continuously charge the battery.
   - The active charging indicator animation (blink, pulse, or solid glow) SHALL be rendered on the resolved charging pin.
   - USB Serial/JTAG communications SHALL remain responsive.
3. **Power Button Exit ("Switch On")**: When the physical power button is pressed/clicked while in `CHARGING_ACTIVE` or `CHARGING_COMPLETE`:
   - The board SHALL transition to `ON` (RUN mode).
   - The RadioKit BLE stack and audio synthesis engine SHALL initialize and start.
4. **Unplug Power-Off**: If the vehicle is disconnected from USB power while in `CHARGING_ACTIVE` or `CHARGING_COMPLETE` without having been switched `ON`:
   - The system SHALL blink the charging indicator (or headlight / indicator pin fallback) 4 times quickly (200ms cycle: 100ms ON / 100ms OFF) while keeping `POWER_ENABLE` latched.
   - Immediately following the 4th blink, the system SHALL execute `HardwareInit::powerOff()` to release `POWER_ENABLE` and safely power down the board.
5. **Switching OFF while USB Connected**: If a power-down event (long power button hold or disconnect idle timeout) occurs while USB power is connected:
   - The system SHALL NOT enter dormant sleep or pull `POWER_ENABLE` low.
   - The system SHALL terminate the operational stack (stop BLE advertising and disconnect clients, stop audio output, halt motors, sleep servos) and transition into `CHARGING_ACTIVE` or `CHARGING_COMPLETE`.
   - `POWER_ENABLE` SHALL remain latched ON to continue battery charging.

#### Scenario: Boot into minimal charging mode upon USB connection
- **WHEN** board boots from USB connection with power button unpressed
- **THEN** board latches power, enters `CHARGING_ACTIVE` (or `CHARGING_COMPLETE` if battery is full), leaves BLE modem and audio synthesis disabled, and suspends disconnect timeout

#### Scenario: User switches vehicle ON from charging mode
- **WHEN** user clicks or presses the physical power button while in charging mode
- **THEN** board transitions to `ON` state, starts RadioKit BLE advertising, initializes audio output, and enters normal operational control

#### Scenario: USB unplugged while in charging mode
- **WHEN** USB power is removed while the board remains in charging mode without being switched ON
- **THEN** board blinks the charging indicator 4 times quickly (200ms cycle) and then shuts down via `HardwareInit::powerOff()`

#### Scenario: Power-off triggered while USB is connected
- **WHEN** power button is held for 4s (or disconnect timeout elapses) while USB power is connected
- **THEN** board shuts down the operational stack (BLE modem, audio, motors/servos) and transitions directly to charging mode with `POWER_ENABLE` latched ON
