## Why

Currently, the RadioKit device name and description are hardcoded in `src/RADIOKIT.h` (or derived partially without a unified fallback cascade). Users need the ability to customize the device's advertised BLE name and description per board hardware config, per vehicle profile bundle, or fall back to the compile-time designer header defaults.

## What Changes

- Add top-level `name` and `description` fields to `HardwareConfig` and `configs/schemas/hardware_config.schema.json`.
- Add `description` support to `RcEngineSound::Config` in `lib/SoundEngine` and `ConfigParser::loadVehicleConfig` (under `vehicle.description`).
- Implement a hierarchical fallback cascade for device name and description:
  1. **Hardware Config JSON** (`hardware-<BOARD>.json` top-level `name` / `description`)
  2. **Vehicle Config JSON** (`vehicle.json` -> `vehicle.name` / `vehicle.description`)
  3. **RADIOKIT.h** (`initRadioKit()` designer fallback defaults)
- Apply the resolved metadata to `RadioKit.config.name` and `RadioKit.config.description` at boot before `RadioKit.begin()` / `startBLE()`.
- Update metadata dynamically during LittleFS hot-reload (`reloadConfigs()`), updating `RadioKitBLE.updateAdvertisingName()` if BLE is running.
- Update `scripts/validate_configs.py` and hardware schema to recognize optional top-level `name` and `description`.

## Capabilities

### Modified Capabilities
- `radiokit-ble-control`: Updates RadioKit lifecycle and device metadata requirements to follow the 3-tier cascade (Hardware Config > Vehicle Config > RADIOKIT.h fallback) for device name and description at boot and hot-reload.
- `config-schema-validation`: Adds top-level optional `name` (max 32 chars) and `description` (max 128 chars) properties to hardware config schema and vehicle parser.

## Impact

- `common/Config.h`: `HardwareConfig` gains `char name[32]` and `char description[128]`.
- `lib/SoundEngine/src/RcEngineSound.h`: `RcEngineSound::Config` gains `char description[128]`.
- `common/ConfigParser.h`: Parses top-level `name` and `description` from hardware config, and `vehicle.description` from vehicle config; updates key checkers.
- `src/main.cpp`: Applies cascading resolution at startup and on hot reload.
- `configs/schemas/hardware_config.schema.json`: Adds schema definitions for `name` and `description`.
- `scripts/validate_configs.py`: Validates hardware configs containing `name` and `description`.
