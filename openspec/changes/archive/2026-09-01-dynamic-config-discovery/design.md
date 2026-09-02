## Context

On ESP32 LittleFS, configuration files for RC_Engine include one hardware configuration and one vehicle configuration. Previously, `src/main.cpp` used compile-time macros (`HW_CONFIG_PATH`) with static paths like `/hardware-MIKRO_V2.json` and a fixed `/vehicle-config.json`. When files were named with variants (such as `/hardware-MIKRO_V2-truck.json` or `/vehicle-ScaniaV8.json`) or deployed within bundle folders (`/vehicle_configs/ScaniaV8/vehicle.json`), the firmware failed to locate them at boot or during hot-reload.

## Goals / Non-Goals

**Goals:**
- Implement `ConfigParser::findHardwareConfig()` to search `/` for the first file matching `hardware-*.json`.
- Implement `ConfigParser::findVehicleConfig()` to search `/` for `vehicle.json` / `vehicle-*.json` and search `/vehicle_configs/` / `/vehicle_config/` subdirectories.
- Provide pathless `loadHardwareConfig(config)` and `loadVehicleConfig(config)` methods.
- Update `src/main.cpp` `setup()` and `reloadConfigs()` to use dynamic discovery and log discovered filenames.

**Non-Goals:**
- Supporting multiple active hardware configurations simultaneously on a single boot (exactly one hardware config and one vehicle config are selected).

## Decisions

1. **Hardware Config Search Strategy**:
   - *Decision*: Scan LittleFS root directory `/`. Find the first entry where `!entry.isDirectory()`, filename starts with `"hardware-"` (or `"/hardware-"`) and ends with `".json"`.
   - *Rationale*: Clean, simple, deterministic, and compatible with all board names and variant suffixes.

2. **Vehicle Config Search Strategy**:
   - *Decision*:
     1. First scan root `/`: match `vehicle.json` or any `vehicle-*.json`.
     2. If not found in root, scan `/vehicle_configs/` and `/vehicle_config/`: check `<dir>/vehicle.json` or `<dir>/vehicle-*.json`, or files within nested vehicle bundle folders.
   - *Rationale*: Accommodates both flat files (`/vehicle-config.json`, `/vehicle.json`) and structured bundle layouts (`/vehicle_configs/<name>/vehicle.json`).

3. **Normalized Absolute Path Return**:
   - *Decision*: Discovery methods always return a valid absolute path beginning with `/` (e.g. `"/hardware-MIKRO_V2-truck.json"`), or an empty `String("")` if no match was found.

## Risks / Trade-offs

- **[Risk] Directory Traversal Overhead** → Negligible: LittleFS root in RC_Engine typically contains fewer than 10 entries; directory traversal completes in < 2 ms at boot.
