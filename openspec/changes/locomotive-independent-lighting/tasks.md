# Tasks: Locomotive Independent Lighting & UI Sync

## 1. UI Design Sync
- [x] 1.1 Sync `docs/radiokit-rc-ui-design.json` and `src/RADIOKIT.h` with `rc_ui_default` from app

## 2. Firmware Implementation
- [x] 2.1 Simplify locomotive headlight logic in `VehicleController.h` (remove interlock to Bit 1)
- [x] 2.2 Make ditch lights purely manual on Bit 2 (`0x04`) in `applyLightsWithAutomation`
- [x] 2.3 Align Cab (Bit 4) and Step (Bit 5) light masks

## 3. Host Tests & Build Verification
- [x] 3.1 Run `host_vc` and `host_dsp` test suites
- [x] 3.2 Build firmware for `TRACKLINK_V3`
- [x] 3.3 Flash firmware to board and verify over BLE via RadioKit Remote API
