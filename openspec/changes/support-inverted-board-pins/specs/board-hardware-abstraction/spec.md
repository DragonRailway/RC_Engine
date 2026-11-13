## MODIFIED Requirements

### Requirement: Self-contained board definition contract
Every board definition in `boards/` SHALL be fully self-contained, encapsulating its pin vocabulary (`PINS[]`), motor drivers (`DRIVERS[]`), audio DAC (`AUDIO`), and power management (`POWER`).
Boards SHALL represent pins using signed 8-bit integers (`int8_t`).
Boards SHALL inherit safe disabled defaults (`NO_PIN = 127` / empty) from `BaseBoard`, allowing peripherals that do not exist on the board to be completely omitted.
Negative pin numbers (e.g. `-42`) SHALL designate active-low / inverted polarity for that hardware connection.
Power accessory pins SHALL use standard semantic names: `SERVO_ENABLE` (for 5V buck/servo rail gating) and `PUMP_ENABLE` (for high-side MOSFET/pump output gating).

#### Scenario: Board defines pins, drivers, and peripherals
- **WHEN** a board header defines `PINS[]`, `DRIVERS[]`, `AUDIO`, and `POWER`
- **THEN** all GPIO constants, named aliases, and driver configurations are resolved directly from that board definition without referencing external `#ifdef` blocks

#### Scenario: Board defines power accessory control pins
- **WHEN** a board header defines `POWER::SERVO_ENABLE` and `POWER::PUMP_ENABLE` (e.g. GTRACK)
- **THEN** power accessory GPIOs are accessed via these standard pin constants and initialized accordingly in `HardwareInit`

#### Scenario: Board omits optional peripherals
- **WHEN** a board header omits `AUDIO`, `DRIVERS`, or `POWER`
- **THEN** the board inherits `BaseBoard` defaults where unconfigured pins equal `NO_PIN` (`127`) and drivers array is empty

### Requirement: Unified Board API for firmware subsystems
The umbrella header `boards/boards.h` SHALL provide a unified `Board` API that resolves named string identifiers to signed pin numbers (`int8_t`), provides compile-time feature checks, and provides pin polarity resolution helpers (`isPinInverted`, `toGpio`, `isValid`).
`NO_PIN` SHALL be defined as `static constexpr int8_t NO_PIN = 127;`.
No board-specific `#ifdef <BOARD_NAME>` blocks SHALL exist in `common/` files (`ConfigParser.cpp`, `HardwareInit.cpp`, `PinMapper.h`).

#### Scenario: Resolving valid pin names
- **WHEN** `Board::resolve("L1")` or `Board::resolve("S1")` is called
- **THEN** it returns the corresponding `int8_t` pin value for the active board

#### Scenario: Resolving invalid or unknown pin names
- **WHEN** `Board::resolve("UNKNOWN")` or a null string is passed
- **THEN** it returns `NO_PIN` (`127`)

#### Scenario: Audio capability detection
- **WHEN** `Board::hasAudio()` is called on a board with audio pins defined
- **THEN** it evaluates to `true`, and evaluates to `false` when audio pins are omitted or set to `NO_PIN`

## ADDED Requirements

### Requirement: Inverted pin representation and hardware polarity resolution
The board system SHALL support negative pin numbers (`pin < 0`) in `PINS[]`, `POWER`, and `DRIVERS[]` to designate inverted/active-low hardware pins.
`Board::isPinInverted(int8_t pin)` SHALL return `true` if and only if `pin < 0`.
`Board::toGpio(int8_t pin)` SHALL return `NO_PIN` (or `0xFF` when mapped to unsigned 8-bit GPIO) if `pin == NO_PIN`, otherwise `static_cast<uint8_t>(abs(pin))`.
Firmware initialization subsystems SHALL propagate the inversion state:
- Lighting channels configured with `EasyLED` SHALL set `EasyKit::LEDConfig::invertOutput = Board::isPinInverted(pin)`.
- Fallback digital writes and power/accessory control pins (`POWER_ENABLE`, `SERVO_ENABLE`, `PUMP_ENABLE`) SHALL invert the output logic level when the pin is declared negative.
- Input signals (`CHARGE_SENS`, `POWER_BUTTON`) SHALL invert the detected logic level when the pin is declared negative.

#### Scenario: Resolving an active-low named LED
- **WHEN** a board defines `{"L0", -42}` and `Board::resolve("L0")` is called
- **THEN** `resolve("L0")` returns `-42`, `Board::isPinInverted(-42)` returns `true`, and `Board::toGpio(-42)` returns `42`

#### Scenario: Resolving an active-high named pin
- **WHEN** a board defines `{"L1", 11}` and `Board::resolve("L1")` is called
- **THEN** `resolve("L1")` returns `11`, `Board::isPinInverted(11)` returns `false`, and `Board::toGpio(11)` returns `11`

#### Scenario: Active-low power enable pin
- **WHEN** a board defines `POWER::POWER_ENABLE = -33`
- **THEN** `Board::isPinInverted(POWER::POWER_ENABLE)` returns `true`, and setting the power latch to active drives GPIO 33 `LOW` instead of `HIGH`
