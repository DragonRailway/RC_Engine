# RadioKit BLE Control

## Why

RC_brain currently has no remote control input path: `src/main.cpp` initializes hardware and the sound engine, then `loop()` is empty. A RadioKit Flutter companion app is running on the local network (Android, `10.0.0.6:7007`) with a saved **RC_UI** design (id `1785927365527`) — a two-page truck/locomotive control surface — ready to be consumed by the firmware. RadioKit v2.0.0 already exists on this machine at `/home/sun/Apps/RadioKit/rk-arduino`.

## What Changes

- Add **RadioKit v2.0.0** as a local library dependency in `platformio.ini` (symlink to `/home/sun/Apps/RadioKit/rk-arduino`, same pattern as `ESP32_PWM_Fusion`)
- Add **`src/RADIOKIT.h`** generated from the saved RC_UI design: 2 pages (Truck, Loco), 8 widgets, 2 telemetry widgets, BLE transport, OTA + filesystem features enabled
- Implement a **vehicle control loop** in `src/main.cpp` that maps widget inputs to motor PWM, steering servo, lights, and the sound engine, and feeds battery/speed telemetry back to the app
- Enable **RadioKit remote filesystem** (`RK_ENABLE_FS` + `RadioKit.enableFS()`) so `/hardware-config.json`, `/vehicle-config.json`, and `/sounds/*.json` can be browsed/edited/uploaded from the app
- Add a **config reload** path that re-parses configs from LittleFS and hot-reloads hardware/audio without reboot

## Capabilities

### New Capabilities
- `radiokit-ble-control`: RadioKit library integration — RADIOKIT.h widget declarations from the RC_UI design, library init/lifecycle (`begin`, `update`, transports), and BLE/Serial transport startup
- `vehicle-control-loop`: Runtime control loop mapping RadioKit widget inputs (throttle, steering, brakes, direction, lights, horn) to motor, servo, light, and sound engine outputs, plus telemetry reporting
- `config-filesystem-management`: Remote LittleFS management through RadioKit (browse/read/write/upload configs and sound JSONs) and reload of hardware/vehicle configs at runtime

### Modified Capabilities
- None (existing specs `unified-firmware-entrypoint`, `littlefs-config-consolidation`, `sound-engine-config` do not change at the requirement level)

## Impact

- **`platformio.ini`**: add `RadioKit=symlink:///home/sun/Apps/RadioKit/rk-arduino` to `lib_deps`; no build flags needed (feature defines live in RADIOKIT.h)
- **`src/main.cpp`**: replace empty `loop()` with the control loop; wire `initRadioKit()` into `setup()`; keep the existing boot/config/sound initialization intact
- **`src/RADIOKIT.h`** (new): generated header from the RC_UI design — self-registering widget globals, `initRadioKit()` helper
- **`common/`**: `HardwareInit` currently only initializes pins; a small runtime motor/servo/light control layer is needed (or direct `ledc` writes) for the control loop
- **`SoundEngine`**: the engine exposes config + sound playback; the control loop drives it (throttle → RPM/pitch, horn/brake triggers) via its existing public API
- **Dependency**: RadioKit v2.0.0 (MIT, `rambros3d/RadioKit`) — present locally, not yet wired into the build
- **No breaking changes** to the boot sequence or config schema
