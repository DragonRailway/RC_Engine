## 1. Data Model & Config Structures

- [x] 1.1 Add `char name[32]` and `char description[128]` to `HardwareConfig` in `common/Config.h`
- [x] 1.2 Add `char description[128]` to `RcEngineSound::Config` in `lib/SoundEngine/src/RcEngineSound.h`
- [x] 1.3 Update `configs/schemas/hardware_config.schema.json` to allow optional top-level `name` (max 32 chars) and `description` (max 128 chars)

## 2. Config Parsing & Validation

- [x] 2.1 Update `ConfigParser::loadHardwareConfig` in `common/ConfigParser.h` to parse top-level `name` and `description`
- [x] 2.2 Update `ConfigParser::loadVehicleConfig` in `common/ConfigParser.h` to parse `vehicle.description`
- [x] 2.3 Update `checkUnknownHardwareKeys` and `checkUnknownVehicleKeys` in `common/ConfigParser.h` to allow `name` and `description`
- [x] 2.4 Verify with `python3 scripts/validate_configs.py`

## 3. RadioKit Metadata Resolution & Lifecycle

- [x] 3.1 Implement `applyDeviceMetadata` in `src/main.cpp` implementing the 3-tier cascade (Hardware Config > Vehicle Config > RADIOKIT.h)
- [x] 3.2 Invoke `applyDeviceMetadata` in `src/main.cpp` `setup()` before `RadioKit.begin()` / `startBLE()`
- [x] 3.3 Invoke `applyDeviceMetadata` in `src/main.cpp` `reloadConfigs()` and call `RadioKitBLE.updateAdvertisingName()` on hot-reload
- [x] 3.4 Build firmware with `pio run -e TRACKLINK_V3` and test with `pio test` or test harness
