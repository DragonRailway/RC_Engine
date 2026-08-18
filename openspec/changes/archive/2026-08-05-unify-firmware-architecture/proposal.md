## Why

Currently, the RC Brain firmware is split into two separate directories (`RC_Loco/` and `RC_Truck/`) with duplicated entrypoints (`main.cpp`) and separate PlatformIO build environments (`[env:RC_Loco]` and `[env:RC_Truck]`). Vehicle type is hardcoded at compile time via preprocessor build flags (`-D RC_TRUCK`, `-D RC_LOCO`). 

This duplicate structure creates maintenance overhead and prevents deploying a single, universal firmware image to ESP32-S3 target boards. Unifying the firmware into a single `src/main.cpp` entrypoint with runtime vehicle type and parameter selection from LittleFS config files simplifies deployment and configuration.

## What Changes

- **BREAKING**: Remove duplicate `RC_Loco/` and `RC_Truck/` folders, consolidating code into a single unified `src/main.cpp` entrypoint.
- **BREAKING**: Update `platformio.ini` to define a single default build environment `[env:TRACKLINK_V3]` (removing split `[env:RC_Truck]` and `[env:RC_Loco]` environments and compile-time `-D RC_TRUCK`/`-D RC_LOCO` flags).
- **Runtime Vehicle Configuration**: Extend `ConfigParser` to read `VEHICLE.TYPE` dynamically from `/vehicle-config.json` at boot into a runtime `VehicleType` enum (`TRUCK`, `LOCOMOTIVE`, `EXCAVATOR`, `CUSTOM`).
- **Canonical Config Resolution (Option B)**: Firmware at boot always loads `/hardware-config.json` and `/vehicle-config.json` from LittleFS root.
- **Sound Resolution Strategy**: Sound file loader attempts vehicle-prefixed sound files (`/sounds/{NAME}-{type}.json`) first, falling back to generic sound files (`/sounds/{type}.json` or standard generic names).
- **Boot Safety**: If LittleFS mounting fails or required config files are missing/corrupted at startup, execution halts with a fatal log on Serial (2,000,000 baud) while keeping all outputs disabled.

## Capabilities

### New Capabilities
- `unified-firmware-entrypoint`: Single main entrypoint and build configuration for all supported vehicle types, resolving hardware and vehicle profiles dynamically at startup.

### Modified Capabilities
- `littlefs-config-consolidation`: Modify requirement so `VEHICLE.TYPE` is parsed at runtime from `/vehicle-config.json` instead of being specified via compile-time build flags.

## Impact

- **Build System**: `platformio.ini` simplified to target `src/main.cpp` under a unified board environment `[env:TRACKLINK_V3]`.
- **Directory Structure**: `RC_Loco/` and `RC_Truck/` subdirectories removed.
- **Config & Sound Engine**: `ConfigParser` updated to parse `VEHICLE.TYPE` string into `RcEngineSound::Config` or `VehicleType` enum and load sounds cleanly.
- **Backwards Compatibility**: Flash filesystem structure remains LittleFS with standard `/hardware-config.json` and `/vehicle-config.json`.
