# Tasks: uint16_t Power Time Parameters

## Section 1: Schema, Config & Firmware Updates

- [x] 1.1 Update `configs/schemas/hardware_config.schema.json`: change `power` timing properties to `"type": "integer"`
- [x] 1.2 Update `common/Config.h`: change `HardwareConfig::Power` time fields to `uint16_t`
- [x] 1.3 Update `common/ConfigParser.h`: parse integer values and constrain `uint16_t` ranges
- [x] 1.4 Update `common/HardwareInit.h` & `common/VehicleController.h`: update method signatures and `millis()` calculations for `uint16_t` seconds
- [x] 1.5 Update shipped hardware JSONs (`configs/hardware_configs/hardware-*.json`) to use integer seconds values

## Section 2: Documentation & Verification

- [x] 2.1 Update `GUIDE/HARDWARE_CONFIG.md` section 8 to document integer `uint16_t` seconds parameters
- [x] 2.2 Run `python3 scripts/validate_configs.py` to confirm hardware configs pass schema validation
- [x] 2.3 Update `test/host_vc/host_vc_driver.cpp` and run `python3 scripts/host_vc_test.py` to verify all 13 test suites pass
- [x] 2.4 Build firmware environments (`pio run`, `pio run -e MIKRO_V2`) to ensure 0 compilation errors
