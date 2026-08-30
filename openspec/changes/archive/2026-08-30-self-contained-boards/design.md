## Context

Historically, board pin definitions were split across two locations:
1. `boards/<BOARD>.h`: Contained compile-time integer constants for GPIOs (`PIN`, `DRIVER`, `AUDIO`, `POWER`).
2. `common/PinMapper.h`: Contained string-to-GPIO resolution (`"L1"`, `"S1"`), driver construction (`"DRIVER_A"` / `"DRIVER_B"`), and cascading `#ifdef` branches for every supported board.

This tight coupling meant that adding or modifying a board required editing files in both `boards/` and `common/`, and boards with non-standard hardware topologies (e.g. no I2S audio, no H-bridge drivers, or PWM+DIR drivers) were difficult to model cleanly.

## Goals / Non-Goals

**Goals:**
- Make every board definition in `boards/` completely self-contained (pin map, driver map, audio, power).
- Provide clean, self-documenting driver initializers (`Driver::DualPWM`, `Driver::PwmDir`) with named fields (`.pwm1`, `.pwm2`, `.enable`, `.bemf`).
- Allow boards lacking certain peripherals (audio DAC, H-bridges, power buttons) to omit them cleanly without boilerplate or dummy `0xFF` constants.
- Provide a unified `Board` API in `boards/boards.h` for firmware subsystems (`Board::resolve()`, `Board::getDriver()`, `Board::hasAudio()`, `Board::hasDrivers()`).
- Eliminate all board-specific `#ifdef` branches from `common/` (`PinMapper.h`, `ConfigParser.cpp`, `HardwareInit.cpp`).
- Maintain 100% backward compatibility for JSON configs and host tests.

**Non-Goals:**
- Changing existing JSON configuration keys (`"L1"`, `"DRIVER_A"`, etc.).
- Modifying physical pin assignments on existing boards (`TRACKLINK_V3`, `MIKRO_V2`, `GTRACK`).

## Decisions

### 1. Unified Board Types and Base Defaults (`BoardTypes.h` & `BoardBase.h`)
Define core structures in `boards/BoardTypes.h`:
- `PinEntry`: `{ const char* name; uint8_t pin; }`
- `DriverPins`: `{ uint8_t pwm1, pwm2, enable, bemf; bool dualPwm; }`
- `DriverEntry`: `{ const char* name; DriverPins pins; }`

Provide default empty/disabled initializers in `boards/BoardBase.h`:
- `PINS` defaults to empty array `{}`.
- `DRIVERS` defaults to empty array `{}`.
- `AUDIO` and `POWER` default to `0xFF` for all pin fields.

Boards inherit from `BaseBoard` and only define what is physically present.

### 2. Auto-Inferred Driver Initializers (`Driver::DualPWM` & `Driver::PwmDir`)
Instead of manual mode enums or positional arguments, use designated initializer structs:
```cpp
static constexpr DriverEntry DRIVERS[] = {
    {
        .name = "DRIVER_A",
        .pins = Driver::DualPWM {
            .pwm1   = 13,
            .pwm2   = 14,
            .enable = 12,
            .bemf   = 4
        }
    }
};
```
- `Driver::DualPWM` sets `dualPwm = true`.
- `Driver::PwmDir` takes `.pwm`, `.dir`, `.enable` and sets `dualPwm = false`.

### 3. Unified `Board` API in `boards/boards.h`
`boards/boards.h` includes the active board and exposes a clean interface:
- `Board::resolve(const char* name) -> uint8_t`: Linear scan over `CurrentBoard::PINS`.
- `Board::getDriver(const char* name) -> DriverPins`: Linear scan over `CurrentBoard::DRIVERS`.
- `Board::hasAudio() -> bool`: Returns `Board::AUDIO::DIN != 0xFF`.
- `Board::hasDrivers() -> bool`: Returns `driverCount() > 0`.
- `Board::hasPowerLatch() -> bool`: Returns `Board::POWER::ENABLE != 0xFF`.

### 4. Compatibility Delegation in `PinMapper.h`
`common/PinMapper.h` becomes a thin delegation wrapper around `Board`:
```cpp
class PinMapper {
public:
    static constexpr uint8_t DRIVER_A = 0xE1;
    static constexpr uint8_t DRIVER_B = 0xE2;

    static uint8_t resolve(const char* name) {
        return Board::resolve(name);
    }
    static DriverPins getDriver(const char* name) {
        return Board::getDriver(name);
    }
    static bool isLED(const char* name) { /* prefix check */ }
    static bool isServo(const char* name) { /* prefix check */ }
    static bool isDriver(const char* name) { /* prefix check */ }
};
```
This ensures that `ConfigParser.cpp`, `HardwareInit.cpp`, and test harnesses continue compiling seamlessly while `common/` is freed from board-specific `#ifdef`s.

## Risks / Trade-offs

- **[Linear scan runtime overhead at boot]** → Boards have 10–25 pin entries; a linear scan takes <1 microsecond at boot and only runs during config parsing / hot-reload.
- **[Compiler designated initializers support]** → C++20 / GNU C++ extensions natively supported by ESP32 PlatformIO GCC toolchain.
- **[Host unit tests (`test/host_vc`)]** → Host tests define dummy board configurations that seamlessly implement the `Board` contract.
