## 1. PinMapper & Constants

- [x] 1.1 Add `DRIVER_C = 0xE3` and `DRIVER_D = 0xE4` to `common/PinMapper.h` and update `resolve()` and `isDriver()`

## 2. HardwareInit Driver & Servo Pools

- [x] 2.1 Refactor `common/HardwareInit.h` and `HardwareInit.cpp` to manage an array of 4 `EasyMotor` instances (`s_motorDrivers[4]`)
- [x] 2.2 Update `HardwareInit::init()` and `initAuxOutputs()` to map any `DRIVER_A` through `DRIVER_D` to its corresponding pool slot
- [x] 2.3 Update `HardwareInit::stopAll()` to stop all 4 driver instances

## 3. Verification & Testing

- [x] 3.1 Run multi-board PlatformIO builds across all board environments (`TRACKLINK_V3`, `MIKRO_V2`, `GTRACK`)
- [x] 3.2 Run host tests (`python3 scripts/host_vc_test.py`) and config validation (`python3 scripts/validate_configs.py`)
