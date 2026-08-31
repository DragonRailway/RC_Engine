## ADDED Requirements

### Requirement: Board Voltage Divider Resistor Parameters
Every board definition in `boards/` SHALL specify its physical ADC voltage divider resistors `VOLTAGE_DIV_R_HIGH` and `VOLTAGE_DIV_R_LOW` within its `struct POWER`. `BoardBase` SHALL provide a compile-time helper `computeVoltageDividerRatio(rHigh, rLow)` that calculates $(R_{\text{high}} + R_{\text{low}}) / R_{\text{low}}$. If `VOLTAGE_SENS` is defined but resistor values are omitted ($R_{\text{low}} \le 0$), the ratio SHALL default to `2.0` (1:1 equal divider).

#### Scenario: Board specifies explicit resistor divider values
- **WHEN** a board header defines `VOLTAGE_DIV_R_HIGH = 20.0f` and `VOLTAGE_DIV_R_LOW = 5.1f` (e.g. GTRACK)
- **THEN** `Board::POWER::DIVIDER_RATIO` evaluates to $(20.0 + 5.1) / 5.1 \approx 4.92157$

#### Scenario: Board defines voltage sense pin without explicit resistors
- **WHEN** a board defines `VOLTAGE_SENS != 0xFF` but omits or sets `VOLTAGE_DIV_R_LOW = 0.0f`
- **THEN** `Board::POWER::DIVIDER_RATIO` defaults to `2.0f`

#### Scenario: Board with no voltage sense pin
- **WHEN** a board defines `VOLTAGE_SENS = 0xFF`
- **THEN** `Board::POWER::DIVIDER_RATIO` evaluates to `0.0f`

## MODIFIED Requirements

### Requirement: Self-contained board definition contract
Every board definition in `boards/` SHALL be fully self-contained, encapsulating its pin vocabulary (`PINS[]`), motor drivers (`DRIVERS[]`), audio DAC (`AUDIO`), and power management (`POWER`).
Boards SHALL inherit safe disabled defaults (`0xFF` / empty) from `BaseBoard`, allowing peripherals that do not exist on the board to be completely omitted.
Power accessory pins SHALL use standard semantic names: `SERVO_ENABLE` (for 5V buck/servo rail gating) and `PUMP_ENABLE` (for high-side MOSFET/pump output gating).

#### Scenario: Board defines pins, drivers, and peripherals
- **WHEN** a board header defines `PINS[]`, `DRIVERS[]`, `AUDIO`, and `POWER`
- **THEN** all GPIO constants, named aliases, and driver configurations are resolved directly from that board definition without referencing external `#ifdef` blocks

#### Scenario: Board defines power accessory control pins
- **WHEN** a board header defines `POWER::SERVO_ENABLE` and `POWER::PUMP_ENABLE` (e.g. GTRACK)
- **THEN** power accessory GPIOs are accessed via these standard pin constants and initialized accordingly in `HardwareInit`

#### Scenario: Board omits optional peripherals
- **WHEN** a board header omits `AUDIO`, `DRIVERS`, or `POWER`
- **THEN** the board inherits `BaseBoard` defaults where unconfigured pins equal `0xFF` and drivers array is empty
