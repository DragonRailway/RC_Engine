# board-hardware-abstraction Specification

## Purpose
TBD - created by archiving change self-contained-boards. Update Purpose after archive.
## Requirements
### Requirement: Self-contained board definition contract
Every board definition in `boards/` SHALL be fully self-contained, encapsulating its pin vocabulary (`PINS[]`), motor drivers (`DRIVERS[]`), audio DAC (`AUDIO`), and power management (`POWER`).
Boards SHALL inherit safe disabled defaults (`0xFF` / empty) from `BaseBoard`, allowing peripherals that do not exist on the board to be completely omitted.

#### Scenario: Board defines pins, drivers, and peripherals
- **WHEN** a board header defines `PINS[]`, `DRIVERS[]`, `AUDIO`, and `POWER`
- **THEN** all GPIO constants, named aliases, and driver configurations are resolved directly from that board definition without referencing external `#ifdef` blocks

#### Scenario: Board omits optional peripherals
- **WHEN** a board header omits `AUDIO`, `DRIVERS`, or `POWER`
- **THEN** the board inherits `BaseBoard` defaults where unconfigured pins equal `0xFF` and drivers array is empty

---

### Requirement: Declarative motor driver topologies with self-documenting syntax
The board contract SHALL support declarative motor driver initializers (`Driver::DualPWM`, `Driver::PwmDir`) with named field initializers (`.pwm1`, `.pwm2`, `.enable`, `.bemf`, `.pwm`, `.dir`) that automatically infer driver control topology.

#### Scenario: Dual-PWM driver configuration
- **WHEN** a board declares a driver using `Driver::DualPWM { .pwm1 = 13, .pwm2 = 14, .enable = 12, .bemf = 4 }`
- **THEN** `Board::getDriver("DRIVER_A")` returns a `DriverPins` struct with `dualPwm = true`, `pwm1 = 13`, `pwm2 = 14`, `enable = 12`, and `bemf = 4`

#### Scenario: PWM and direction driver configuration
- **WHEN** a board declares a driver using `Driver::PwmDir { .pwm = 18, .dir = 19, .enable = 17 }`
- **THEN** `Board::getDriver("DRIVER_A")` returns a `DriverPins` struct with `dualPwm = false`, `pwm1 = 18`, `pwm2 = 19`, `enable = 17`, and `bemf = 0xFF`

#### Scenario: Board with no H-bridge drivers
- **WHEN** a board definition does not declare any `DRIVERS`
- **THEN** `Board::hasDrivers()` evaluates to `false` and `Board::getDriver(name)` returns an unconfigured driver struct

---

### Requirement: Unified Board API for firmware subsystems
The umbrella header `boards/boards.h` SHALL provide a unified `Board` API that resolves named string identifiers to GPIO numbers and provides compile-time feature checks.
No board-specific `#ifdef <BOARD_NAME>` blocks SHALL exist in `common/` files (`ConfigParser.cpp`, `HardwareInit.cpp`, `PinMapper.h`).

#### Scenario: Resolving valid pin names
- **WHEN** `Board::resolve("L1")` or `Board::resolve("S1")` is called
- **THEN** it returns the corresponding GPIO integer for the active board

#### Scenario: Resolving invalid or unknown pin names
- **WHEN** `Board::resolve("UNKNOWN")` or a null string is passed
- **THEN** it returns `0xFF`

#### Scenario: Audio capability detection
- **WHEN** `Board::hasAudio()` is called on a board with audio pins defined
- **THEN** it evaluates to `true`, and evaluates to `false` when audio pins are omitted or set to `0xFF`

