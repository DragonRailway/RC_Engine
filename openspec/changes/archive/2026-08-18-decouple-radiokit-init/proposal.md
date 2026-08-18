# Proposal: Decouple RadioKit Initialization and Staged BLE Lifecycle

## Problem
In `src/RADIOKIT.h`, the generated function `initRadioKit()` combines widget layout declarations, default configuration assignments (`RadioKit.config.name = "RC_UI"`), configuration committing (`RadioKit.begin()`), and BLE transport startup (`RadioKit.startBLE()`) in a single monolithic call.

This design introduces two critical limitations:
1. **Hardcoded Overwrite & Race Condition**: Any dynamic metadata resolved from `hardware-<BOARD>.json` or `vehicle.json` before `initRadioKit()` is immediately overwritten with `"RC_UI"`.
2. **Premature Advertising**: BLE advertising begins before `src/main.cpp` has a chance to commit the vehicle-specific name (e.g. `"Scania V8"` vs `"Caterpillar 323"`) or board-specific name (e.g. `"Mikro Truck"`).

## Proposed Solution
Decouple `initRadioKit()` into dedicated initialization stages:
1. **Layout & Widget Setup**: `initRadioKit()` configures widget geometry, icons, and page collections without starting transports or clobbering dynamic metadata.
2. **Metadata Cascade Application**: `applyDeviceMetadata(hwConfig, profile.config)` applies the 3-tier cascade (`Hardware -> Vehicle -> Fallback`) to `RadioKit.config.name`, `description`, and `type`.
3. **Transport Lifecycle Start**: `RadioKit.begin()`, `RadioKit.startSerial()`, `RadioKit.startBLE()`, and `RadioKit.enableFS()` are explicitly called in `src/main.cpp` `setup()` only after metadata is committed.
4. **Runtime BLE Name Sync**: During LittleFS hot-reload (`reloadConfigs()`), `NimBLEDevice::getAdvertising()->setName()` is updated if disconnected so subsequent BLE scans immediately reflect the new name.

## Impact & Scope
- `src/RADIOKIT.h`: Clean `initRadioKit()` to focus strictly on widget/page layout registration.
- `src/main.cpp`: Sequence `initRadioKit() -> applyDeviceMetadata() -> RadioKit.begin() -> RadioKit.startBLE() -> RadioKit.enableFS()`.
- `configs/hardware_configs/hardware-MIKRO_V2-truck.json`: Empty `name` defaults to vehicle bundle name (`Scania V8`).
- `test/host_vc/host_vc_driver.cpp`: Update host tests to match the decoupled lifecycle.
