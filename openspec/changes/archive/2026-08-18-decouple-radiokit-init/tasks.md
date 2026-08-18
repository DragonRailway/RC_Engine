# Tasks: Decouple RadioKit Initialization and Staged BLE Lifecycle

## 1. Header & Lifecycle Refactoring
- [x] 1.1 Update `src/RADIOKIT.h` to decouple `initRadioKit()` (remove `RadioKit.begin()`, `startSerial()`, `startBLE()`, `enableFS()`)
- [x] 1.2 Update `src/main.cpp` `setup()` to execute staged initialization: `initRadioKit()` -> `applyDeviceMetadata()` -> `RadioKit.begin()` -> `RadioKit.startSerial()` -> `RadioKit.startBLE()` -> `RadioKit.enableFS()`
- [x] 1.3 Update `applyDeviceMetadata()` in `src/main.cpp` to set `type` / `activePage` and update `NimBLEAdvertising` name at runtime

## 2. Hardware Configs & Validation
- [x] 2.1 Update `configs/hardware_configs/hardware-MIKRO_V2-truck.json` to omit or blank `name` so it cascades to vehicle bundle name
- [x] 2.2 Verify `hardware-TRACKLINK_V3-locomotive.json` and other hardware configs adhere to cascade rules

## 3. Host Tests & Build Verification
- [x] 3.1 Update `test/host_vc/host_vc_driver.cpp` to match decoupled lifecycle
- [x] 3.2 Build firmware for `MIKRO_V2` and `TRACKLINK_V3` (`pio run`)
- [x] 3.3 Run host unit tests (`pio test`)
