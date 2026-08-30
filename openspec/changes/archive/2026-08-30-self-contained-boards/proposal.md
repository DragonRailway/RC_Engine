## Why

Board definitions in `boards/` currently only define raw GPIO integer constants, while pin string name resolution (`"L1"` -> GPIO 6), driver configurations (`"DRIVER_A"` / `"DRIVER_B"`), and board branching logic are centralized inside `common/PinMapper.h` using cascading `#ifdef` blocks. This leaks board-specific implementation details into generic `common/` infrastructure, forces every new board to touch multiple disparate files, and makes it cumbersome to support diverse board topologies (e.g. boards without audio DACs, without H-bridge drivers, or with single PWM channels).

This change encapsulates each board's complete hardware specification (pin vocabulary, motor driver layouts, audio pins, and power management) inside its own self-contained header under `boards/`, enabling true plug-and-play board additions with zero board-specific `#ifdef`s in `common/`.

## What Changes

- **Self-Contained Board Definitions**: Move pin vocabulary resolution (`PINS[]`) and motor driver declarations (`DRIVERS[]`) into each board header (`boards/TRACKLINK_V3.h`, `boards/MIKRO_V2.h`, `boards/GTRACK.h`).
- **Intuitive & Self-Explanatory Driver Syntax**: Introduce declarative driver helpers (`Driver::DualPWM`, `Driver::PwmDir`) with designated field initializers (`.pwm1`, `.pwm2`, `.enable`, `.bemf`) that automatically infer driver topology and make PCB wiring obvious at a glance.
- **Base Defaults & Feature Omission**: Provide a `BaseBoard` with safe defaults (`NO_PIN` / `0xFF`), allowing boards lacking audio DACs, motor drivers, or soft power latches to completely omit those sections without compiler errors or dummy boilerplate.
- **Unified `Board` API**: Create a unified `Board` class in `boards/boards.h` providing compile-time capability checks (`Board::hasAudio()`, `Board::hasDrivers()`, `Board::hasPowerLatch()`) and generic string-to-hardware resolvers (`Board::resolve()`, `Board::getDriver()`).
- **Decouple `common/` Infrastructure**: Refactor `common/ConfigParser.cpp`, `common/HardwareInit.cpp`, and `common/PinMapper.h` to use `Board::*` APIs, removing all `#ifdef BOARD_NAME` branches from `common/`.
- **Preserve Compatibility**: Keep `PinMapper.h` as a lightweight delegation wrapper to `Board` to avoid breaking existing test harnesses and external call sites.

## Capabilities

### New Capabilities
- `board-hardware-abstraction`: Self-contained board definition contract in `boards/` encapsulating pin mappings, motor drivers, audio DAC, and power management with automatic feature detection and zero board-specific `#ifdef`s in `common/`.

### Modified Capabilities
*(None. Existing runtime vehicle and hardware configuration behavior, pin mapping semantics, and power management requirements remain fully preserved.)*

## Impact

- **`boards/`**: New `BoardTypes.h` and `BoardBase.h`; updated `TRACKLINK_V3.h`, `MIKRO_V2.h`, `GTRACK.h`, and `boards.h`.
- **`common/`**: `PinMapper.h` simplified to delegate to `Board`; `ConfigParser.cpp` and `HardwareInit.cpp` interact with `Board` methods without hardcoded board macros.
- **`scripts/`**: `scripts/gen_boards_header.py` and any config validation scripts updated to reference the new board contract if applicable.
- **`test/`**: Host test harnesses (`test/host_vc/`) continue to pass via `Board` or delegated `PinMapper`.
