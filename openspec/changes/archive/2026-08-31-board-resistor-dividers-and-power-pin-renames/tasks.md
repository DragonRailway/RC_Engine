## 1. Board Definitions & Base Helpers

- [x] 1.1 Update `boards/BoardBase.h` to add `computeVoltageDividerRatio` helper, `VOLTAGE_DIV_R_HIGH`, `VOLTAGE_DIV_R_LOW`, `DIVIDER_RATIO`, and rename `BUCK_5V_EN` to `SERVO_ENABLE` and `POWER_OUT` to `PUMP_ENABLE` in `BaseBoard::POWER`.
- [x] 1.2 Update `boards/GTRACK.h` with `VOLTAGE_DIV_R_HIGH = 20.0f`, `VOLTAGE_DIV_R_LOW = 5.1f`, `SERVO_ENABLE = 44`, and `PUMP_ENABLE = 43`.
- [x] 1.3 Update `boards/TRACKLINK_V3.h`, `boards/MIKRO_V2.h`, and `boards/TRACKLINK_V2.h` with `VOLTAGE_DIV_R_HIGH = 100.0f`, `VOLTAGE_DIV_R_LOW = 100.0f`, and `DIVIDER_RATIO`.

## 2. Firmware Infrastructure & Config Parsing

- [x] 2.1 Update `common/Config.h` battery struct comments and field definitions.
- [x] 2.2 Update `common/ConfigParser.cpp` to compute `battery.vScale` using `BOARD::POWER::DIVIDER_RATIO` multiplied by optional `calibration_factor`, and remove legacy `VSCALE` fallback macros.
- [x] 2.3 Update `common/HardwareInit.h` to use `BOARD::POWER::SERVO_ENABLE` and `BOARD::POWER::PUMP_ENABLE`.
- [x] 2.4 Remove legacy `-D VSCALE=...` flags from `platformio.ini`.

## 3. Configs, Schemas, and Verification

- [x] 3.1 Update `configs/schemas/hardware_config.schema.json` to allow optional `calibration_factor` and `voltage_offset`.
- [x] 3.2 Update `configs/hardware_configs/*.json` to remove legacy `voltage_scale` magic numbers.
- [x] 3.3 Update `test/host_vc/host_vc_driver.cpp` to verify battery scaling with calibration factor.
- [x] 3.4 Validate all configs with `python3 scripts/validate_configs.py` and build all board environments with `pio run`.
