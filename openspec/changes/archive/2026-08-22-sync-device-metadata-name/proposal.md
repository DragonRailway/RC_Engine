## Why

When the RC Brain firmware boots or hot-reloads configs from LittleFS, `applyDeviceMetadata()` evaluates the 3-tier cascade (`Hardware Config -> Vehicle Config -> Fallback`) and assigns `RadioKit.config.name`. However, `RadioKit.begin()` loads cached device settings from ESP32 NVS flash storage (`rk_name`), which overrides `RadioKit.config.name` with stale test/factory strings (e.g. `"RC_UI"`). This causes the app and BLE scanner to show incorrect device names instead of the configured vehicle name (e.g. `"Scania V8"`).

## What Changes

- Update `applyDeviceMetadata()` in `src/main.cpp` to explicitly invoke `RadioKit.setName(targetName)` during both initial `setup()` and runtime `reloadConfigs()`.
- Ensure that the dynamic vehicle name and BLE advertising name are immediately synchronized with the active vehicle/hardware config, overcoming any stale NVS cache.

## Capabilities

### Modified Capabilities
- `radiokit-ble-control`: Guarantee that `RadioKit.setName(targetName)` is invoked in `applyDeviceMetadata()` so that `CONF_DATA` packets, internal `_nvsName`, and live BLE advertisements strictly reflect the resolved metadata cascade name.

## Impact

- Affected files: `src/main.cpp`.
- Transport/System impact: BLE advertising packets and `CONF_DATA` name payloads will immediately match the active vehicle profile (`"Scania V8"`, `"Caterpillar 323"`, etc.) without requiring manual NVS erasing.
