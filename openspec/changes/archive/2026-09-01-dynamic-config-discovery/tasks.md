## 1. ConfigParser Dynamic Discovery Implementation

- [x] 1.1 Declare `findHardwareConfig()`, `findVehicleConfig()`, and zero-argument `loadHardwareConfig(HardwareConfig&)` / `loadVehicleConfig(RcEngineSound::Config&)` in `common/ConfigParser.h`.
- [x] 1.2 Implement `ConfigParser::findHardwareConfig()` in `common/ConfigParser.cpp` to scan LittleFS root (`/`) for the first file matching `hardware-*.json`.
- [x] 1.3 Implement `ConfigParser::findVehicleConfig()` in `common/ConfigParser.cpp` to search root (`/`) for `vehicle.json` / `vehicle-*.json` and search `/vehicle_configs/` and `/vehicle_config/` subdirectories.
- [x] 1.4 Implement `ConfigParser::loadHardwareConfig(HardwareConfig&)` and `ConfigParser::loadVehicleConfig(RcEngineSound::Config&)` using auto-discovery.

## 2. Firmware Entrypoint Clean-up

- [x] 2.1 Remove compile-time `HW_CONFIG_PATH` macro and `#ifdef` board paths from `src/main.cpp`.
- [x] 2.2 Update `src/main.cpp` `setup()` and `reloadConfigs()` to load configs dynamically and log discovered paths.

## 3. Verification & Testing

- [x] 3.1 Run host vehicle controller test harness (`python3 scripts/host_vc_test.py`).
- [x] 3.2 Run host DSP test harness (`python3 scripts/host_dsp_test.py`) and config validation (`python3 scripts/validate_configs.py`).
- [x] 3.3 Compile firmware for `TRACKLINK_V3` and `MIKRO_V2` to verify zero build errors.
