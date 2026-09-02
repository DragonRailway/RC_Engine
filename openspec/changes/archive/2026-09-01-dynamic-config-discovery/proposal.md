## Why

Currently, firmware expects hardcoded, exact configuration paths on LittleFS (such as compile-time `/hardware-<BOARD>.json` and `/vehicle-config.json`). If a filesystem image contains descriptive filenames like `/hardware-MIKRO_V2-truck.json` or `/vehicle-ScaniaV8.json`, or organizes vehicle profiles inside `/vehicle_configs/`, the firmware fails to open them and falls into recovery mode. Dynamic configuration discovery allows the firmware to automatically detect and load the active hardware config and vehicle profile without rigid name constraints.

## What Changes

- **Dynamic Hardware Config Discovery (`ConfigParser::findHardwareConfig`)**:
  - Automatically scan LittleFS root (`/`) for the first file matching `hardware-*.json` and load it.
- **Dynamic Vehicle Config Discovery (`ConfigParser::findVehicleConfig`)**:
  - Automatically search for the first vehicle configuration matching:
    1. Root directory `/`: `vehicle.json` or `vehicle-*.json` (e.g. `vehicle-config.json`, `vehicle-ScaniaV8.json`).
    2. Subdirectories `/vehicle_configs/` and `/vehicle_config/`: `<subdir>/vehicle.json` or `<subdir>/vehicle-*.json`.
- **Zero-Argument Load Overloads**:
  - Add `ConfigParser::loadHardwareConfig(HardwareConfig&)` and `ConfigParser::loadVehicleConfig(RcEngineSound::Config&)` that perform automatic discovery.
- **Main Firmware Clean-up (`src/main.cpp`)**:
  - Remove compile-time `HW_CONFIG_PATH` macro and hardcoded `"/vehicle-config.json"` references, switching boot and hot-reload logic to dynamic auto-discovery.

## Capabilities

### New Capabilities
<!-- None -->

### Modified Capabilities
- `config-filesystem-management`: Update configuration file loading to require dynamic wildcard discovery (`hardware-*.json` and `vehicle-*.json`/subdirectories) rather than static hardcoded file paths.
- `unified-firmware-entrypoint`: Update boot and reload sequences to use dynamic configuration discovery.

## Impact

- **Affected Code**: `common/ConfigParser.h`, `common/ConfigParser.cpp`, `src/main.cpp`.
- **Breaking Changes**: None to data format; removes compile-time `HW_CONFIG_PATH` dependencies in favor of runtime filesystem discovery.
