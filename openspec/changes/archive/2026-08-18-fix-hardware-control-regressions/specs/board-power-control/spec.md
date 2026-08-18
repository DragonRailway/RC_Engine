## MODIFIED Requirements

### Requirement: Boot power latching with 1000ms threshold filter
The firmware SHALL configure `POWER_ENABLE` as an output and `POWER_BUTTON` as an active-HIGH input pin at startup. At boot time in `setup()`, the system SHALL wait for `POWER_BUTTON` to be held HIGH for at least 1000ms before driving `POWER_ENABLE` HIGH to latch hardware power. If `POWER_BUTTON` is released before 1000ms has elapsed, the firmware SHALL NOT drive `POWER_ENABLE` HIGH. If booted via USB (detected by `POWER_BUTTON` LOW at cold boot), `POWER_ENABLE` SHALL be latched HIGH immediately.

Whenever the device detects an active BLE connection (`RadioKit.isConnected()`), the firmware SHALL ensure `POWER_ENABLE` is driven HIGH to guarantee power rails to motor drivers and lighting channels.

#### Scenario: Valid power-on hold (>= 1000ms)
- **WHEN** `POWER_BUTTON` is held HIGH for 1000ms or longer during boot
- **THEN** `POWER_ENABLE` is set HIGH, latching board power ON

#### Scenario: Accidental brief touch (< 1000ms)
- **WHEN** `POWER_BUTTON` is released before 1000ms has elapsed during boot
- **THEN** `POWER_ENABLE` remains LOW, causing board power to turn OFF upon physical button release

#### Scenario: USB boot power latching
- **WHEN** the board boots with `POWER_BUTTON` LOW (powered via USB)
- **THEN** `POWER_ENABLE` is immediately latched HIGH

#### Scenario: Re-latch power rail on BLE connect
- **WHEN** `RadioKit.isConnected()` becomes true
- **THEN** `POWER_ENABLE` is driven HIGH to ensure motor and lighting power rails are active
