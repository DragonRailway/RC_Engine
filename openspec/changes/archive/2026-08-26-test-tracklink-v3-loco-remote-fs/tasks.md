## 1. Vehicle Bundle Creation & Host Validation

- [x] 1.1 Create `configs/vehicle_configs/UnionPacific2002/vehicle.json` with locomotive parameters, EMD 16-710 sound configuration, direct transmission, and engine mass simulation
- [x] 1.2 Validate config bundle assembly and partition sizing using `python3 scripts/build_fs.py --board TRACKLINK_V3 --vehicle UnionPacific2002 --dry-run`

## 2. ADB & RadioKit Remote API Setup

- [x] 2.1 Set up ADB port forwarding (`adb forward tcp:17007 tcp:7007`) and verify endpoint status (`GET /api/status`)
- [x] 2.2 Scan and connect to the TRACKLINK_V3 board over BLE via `/api/pair/scan` and `/api/connection/connect`

## 3. Remote Filesystem Operations & Hot-Reload over BLE

- [x] 3.1 Query LittleFS structure over BLE via `/api/fs/list?path=/` and verify storage statistics via `/api/fs/info`
- [x] 3.2 Upload `hardware-TRACKLINK_V3.json` and `vehicle-config.json` to LittleFS via `/api/fs/upload` or `/api/fs/write`
- [x] 3.3 Verify board hot-reloads the locomotive config and switches active page to Page 1 ("Loco")

## 4. Locomotive Control & Widget Verification

- [x] 4.1 Verify latched engine ignition start/stop via `engine_button` widget
- [x] 4.2 Verify throttle slider and direction switch via `throttle_slider` and `dir_switch`
- [x] 4.3 Verify locomotive lighting bits (headlight, tail, cab, step) and ditch light alternating flash via `loco_light`
- [x] 4.4 Verify locomotive bell sound trigger via `bell_button`
