## 1. Board Headers and Code Generation

- [x] 1.1 Update `boards/BoardTypes.h` with `int8_t` pins and `NO_PIN = 127`
- [x] 1.2 Update `boards/BoardBase.h` with `int8_t` pins in `AUDIO` and `POWER`
- [x] 1.3 Update `scripts/gen_boards_header.py` and regenerate `boards/boards.h` with `isPinInverted()`, `toGpio()`, and `isValid()` helpers

## 2. Common Infrastructure and Config

- [x] 2.1 Update `common/PinMapper.h` with `int8_t` return types and pin polarity helpers
- [x] 2.2 Update `common/Config.h` pin fields to `int8_t`
- [x] 2.3 Update `common/ConfigParser.cpp` pin parsing and `NO_PIN` checks

## 3. Peripheral and Hardware Integration

- [x] 3.1 Update `HardwareInit::initLights()` to pass `invertOutput` to `EasyKit::LEDConfig` and attach `Board::toGpio(pin)`
- [x] 3.2 Update `HardwareInit` direct `digitalWrite` fallback light routines for pin inversion
- [x] 3.3 Update `HardwareInit` power latch and accessory pins (`POWER_ENABLE`, `SERVO_ENABLE`, `PUMP_ENABLE`) to respect polarity
- [x] 3.4 Update `HardwareInit` and `VehicleController` charge sensing (`CHARGE_SENS`) to respect polarity

## 4. Documentation and Build Verification

- [x] 4.1 Update `GUIDE/BOARD_DEFINITION.md` with negative pin inversion documentation
- [x] 4.2 Run PlatformIO builds across environments (`TRACKLINK_V3`, `MIKRO_V2`, `GTRACK`) and test suites
