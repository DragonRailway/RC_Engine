# Design: RadioKit BLE Control

## Context

RC_brain is a unified ESP32-S3 vehicle controller: `src/main.cpp` mounts LittleFS, parses `/hardware-config.json` + `/vehicle-config.json`, initializes hardware via `HardwareInit`, and starts the RcEngineSound audio pipeline. The `loop()` is currently empty — there is no input path.

The RadioKit Flutter companion app (running on the local network at `10.0.0.6:7007`) exposes a REST API and contains a saved design **RC_UI** (id `1785927365527`) tailored for this device: a 2-page (Truck/Loco) control surface with BLE transport, OTA + filesystem features, and Battery/Speed telemetry. The app can generate a ready-to-use `RADIOKIT.h`.

The RadioKit Arduino library **v2.0.0** is already on this machine at `/home/sun/Apps/RadioKit/rk-arduino` (MIT, repo `rambros3d/RadioKit`), but is not yet wired into the build.

Current gaps:
- No runtime motor/servo/light control API — `HardwareInit` only initializes pins (direct `ledcWrite` used at runtime).
- No control loop, no telemetry, no remote config editing.

## Goals / Non-Goals

**Goals:**
- Drive the vehicle from the RC_UI app over BLE (Serial transport as secondary/debug path).
- Map RadioKit widget semantics faithfully: pedals spring-to-min (-100 = released), knob spring-to-center (0 = centered), multi-select bitmasks, momentary horn, non-latching throttle slider.
- Report battery voltage (%) and estimated speed to the app as telemetry.
- Allow remote LittleFS management (browse/edit/upload configs & sound JSONs) and config reload without reboot.
- Preserve the existing boot/init/audio behavior — minimal, additive changes.

**Non-Goals:**
- WiFi / cloud transports (disabled in the RC_UI design).
- Implementing the OTA update flow itself (the feature flag is enabled; driving OTA is app-side).
- Adding new widgets to the app design (ignition, volume slider, etc. — future work if desired).
- Changes to the config JSON schema.

## Decisions

### 1. Library wiring — local symlink `lib_dep`
Add `RadioKit=symlink:///home/sun/Apps/RadioKit/rk-arduino` to `lib_deps` in `platformio.ini`, matching the existing `ESP32_PWM_Fusion` pattern. Feature defines (`RK_ENABLE_OTA`, `RK_ENABLE_FS`, `RK_ENABLE_BLE`) live **inside `RADIOKIT.h`** per the generated-header convention, not in build flags.
- *Alternative rejected*: PlatformIO registry copy of RadioKit — the local v2.0.0 is exactly what the companion app targets and may be ahead of the registry.

### 2. `RADIOKIT.h` generated from the RC_UI design
Place the app-generated header at `src/RADIOKIT.h` (RadioKit convention: sketch dir + RADIOKIT.h). It self-registers 8 widgets across 2 pages plus 2 telemetry widgets, and provides `initRadioKit()` (config → `begin()` → `startSerial(Serial)` → `startBLE()` → `enableFS()`). Archive the design JSON (`GET /api/designs/1785927365527/json`) in the repo (e.g. `docs/radiokit-rc-ui-design.json`) so the header can be regenerated if the app overwrites it.

### 3. Control loop — new header-only `common/VehicleController.h`
Following the project's header-only convention, add a `VehicleController` class that, each loop:
- reads widget state (throttle from gas pedal / Loco slider, steering knob, brake pedal, dir switch, light bitmasks, horn),
- maps -100..+100 into duty cycles / servo µs / ESC µs with clamping and deadband,
- writes motor (H-bridge via `ledcWrite` on PWM1/PWM2 with direction from config; ESC via PPM µs), steering servo, and lights,
- feeds the sound engine (throttle → RPM target; brake/horn triggers),
- publishes telemetry (battery % from voltsense `VSCALE`/`VOFFSET`, speed estimate from throttle).
- *Alternative rejected*: inline everything in `main.cpp` — bloats the entrypoint and mixes concerns.

### 4. Config reload — automatic on file change
`reloadConfigs()` re-runs `ConfigParser::loadHardwareConfig/loadVehicleConfig/loadSounds`, then `HardwareInit::hotReload()` and re-applies the engine config. Trigger: periodic (e.g. 2 s) timestamp check on the config files so saves made in the app's filesystem manager are picked up automatically.
- *Alternative rejected*: a reload button widget — would require editing the app design; keep the RC_UI design as-is.

### 5. Value mapping semantics
- Gas/brake pedals: `RK_SPRING_MIN` → released = **-100**. Throttle % = `(value + 100) / 2`.
- Steering wheel: `RK_SPRING_CENTER` → 0 = center; map -100..+100 to servo L/R endpoints.
- `slider` (Loco page): `RK_SPRING_NONE` — acts as a latching throttle.
- `dir_switch`: bool → FORWARD/REVERSE.
- `led_select` / `lights_toggle`: bitmask → light groups (A/B/C).
- `horn`: momentary → horn sound trigger.
- Telemetry: `telemetry_Battery.rk.content`, `telemetry_Speed.rk.content` updated in loop.

### 6. Transports
BLE is primary (`RadioKit.config.name = "RC_UI"`). `RadioKit.startSerial(Serial)` is also enabled per the generated header — see the baud mismatch risk below.

## Risks / Trade-offs

- **Serial baud mismatch** (boot log at 2,000,000 vs RadioKit config `baudrate = 1000000`) → BLE is the primary transport; keep `baudrate` as generated; align only if Serial transport is actually used.
- **`RadioKit.update()` in a time-sensitive loop** (audio I2S + engine simulation) → measure worst-case loop time; keep FS/OTA handling out of the hot path; `update()` first, then control, then telemetry.
- **Hot reload mid-drive can glitch PWM/audio** → `hotReload()` calls `stopAll()` first; optionally gate reload to throttle-neutral. Brief output mute is acceptable and documented.
- **App may overwrite the generated header** → archive the design JSON so it can be regenerated.
- **4 MB flash budget** (library + app + partitions) → sounds stay on LittleFS; verify build size; `huge_app.csv` partitions already generous.

## Migration Plan

1. Add `lib_dep` + `src/RADIOKIT.h` + archive design JSON → firmware still boots unchanged (loop empty).
2. Add `common/VehicleController.h` and call it from `loop()` → vehicle now drivable; boot sequence untouched.
3. Wire `initRadioKit()` into `setup()` and `RadioKit.update()` into `loop()` → app connectivity.
4. Add config reload check.
- **Rollback**: revert `main.cpp` and remove `lib_dep`; boot/hardware safety is unaffected because the boot sequence is additive.

## Open Questions

- Should config reload be gated on throttle-neutral?
- Keep the RC_UI design exactly as saved, or extend it (ignition, volume, sound profile selector) — the app design can be edited anytime via the API.
- ESC vehicles: confirm PPM-only operation (DSHOT not implemented) is acceptable.
