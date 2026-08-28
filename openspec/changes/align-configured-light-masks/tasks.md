## 1. Core Light Mask Implementation

- [x] 1.1 Update `HardwareInit::getConfiguredLightMask()` in `common/HardwareInit.h` to align locomotive and truck 8-bit masks with UI and controller channel definitions.
- [x] 1.2 Update `GUIDE/HARDWARE_CONFIG.md` lighting bitmask reference table.

## 2. Unit Testing & Host Verification

- [x] 2.1 Update `test/host_vc/host_vc_driver.cpp` to verify `getConfiguredLightMask()` on Truck and Locomotive configurations.
- [x] 2.2 Execute `python3 scripts/host_vc_test.py` and verify all tests pass.

## 3. Firmware Build & Live Hardware Verification

- [x] 3.1 Build firmware for `TRACKLINK_V3` with `pio run -e TRACKLINK_V3`.
- [x] 3.2 Upload firmware to the connected TrackLink V3 board (`pio run -e TRACKLINK_V3 -t upload`).
- [x] 3.3 Query remote RadioKit API (`/api/connection` / `/api/widgets`) to verify only configured buttons (Headlight, Ditch Light, Cab Light, Step Light) are active in `loco_light` `itemMask`.
