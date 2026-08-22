## 1. Implement Device Name Synchronization

- [x] 1.1 Update `applyDeviceMetadata()` in `src/main.cpp` to call `RadioKit.setConfig(RadioKit.config.name, RadioKit.config.description)` so `_nvsName` and NVS flash match the resolved metadata name.
- [x] 1.2 Verify that `setup()` and `reloadConfigs()` both apply the synchronized name before and during BLE transport operation.

## 2. Build & Flash Verification

- [x] 2.1 Compile and flash firmware to `MIKRO_V2` on `/dev/ttyACM0`.
- [x] 2.2 Re-connect the RadioKit app via Remote API and verify the device name displays as `"Scania V8"` on the Active Link card.
