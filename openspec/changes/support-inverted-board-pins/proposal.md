## Why

Different RC boards and circuits use inverted hardware topologies (e.g. active-low status LEDs, active-low buck regulator/servo rail enables, active-low motor driver enables, and active-low charge sense signals). Currently, board definitions only support unsigned GPIO values (`uint8_t`), requiring either bespoke workaround logic or active-high hardware assumptions. Supporting negative pin numbers (e.g. `{"L0", -42}` or `SERVO_ENABLE = -44`) enables declarative, zero-overhead hardware polarity configuration across all board files.

## What Changes

- **Signed Pin Type (`int8_t`)**: Change pin declarations in `boards/` (`PinEntry`, `DriverPins`, `Driver::DualPWM`, `Driver::PwmDir`, `BaseBoard::POWER`, `BaseBoard::AUDIO`) from `uint8_t` to `int8_t`.
- **Sentinel `NO_PIN = 127`**: Update `NO_PIN` from `0xFF` to `127` (`0x7F`) so negative numbers (`-1` .. `-48`) do not collide with `NO_PIN` in signed 8-bit representation.
- **Board Helpers**: Add `Board::isPinInverted(int8_t pin)`, `Board::toGpio(int8_t pin)`, and `Board::isValid(int8_t pin)`.
- **Peripheral Integration**:
  - Update `HardwareInit` light setup (`initGroup`, `EasyLED`) to configure `EasyKit::LEDConfig::invertOutput = Board::isPinInverted(pin)` and bind to `Board::toGpio(pin)`.
  - Update digital output writes (`POWER_ENABLE`, `SERVO_ENABLE`, `PUMP_ENABLE`) and reads (`CHARGE_SENS`, `POWER_BUTTON`) to respect inverted pin polarity.
  - Update motor driver enable pin handling to respect inverted polarity.
- **Header Generation**: Update `scripts/gen_boards_header.py` to generate the new signatures and helper functions in `boards/boards.h`.
- **Documentation**: Update `GUIDE/BOARD_DEFINITION.md` to document negative pin assignment for inverted/active-low hardware.

## Capabilities

### New Capabilities
<!-- None -->

### Modified Capabilities
- `board-hardware-abstraction`: Supports negative `int8_t` pin definitions in board headers (`PINS[]`, `POWER`, `DRIVERS[]`, `AUDIO`), updates `NO_PIN` sentinel to `127`, provides `isPinInverted()` and `toGpio()` helpers, and propagates polarity to peripheral drivers.

## Impact

- **Affected Code**: `boards/BoardTypes.h`, `boards/BoardBase.h`, `boards/boards.h`, `scripts/gen_boards_header.py`, `common/PinMapper.h`, `common/Config.h`, `common/ConfigParser.cpp`, `common/HardwareInit.h`, `common/HardwareInit.cpp`, `GUIDE/BOARD_DEFINITION.md`.
- **Breaking Changes**: None to JSON configs. In C++ firmware, `NO_PIN` is `127` instead of `0xFF`, and code querying pins should use `Board::toGpio()` / `Board::isPinInverted()` or `Board::isValid()`.
