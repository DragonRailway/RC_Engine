# Adopt ESP32_EasyKit as the PWM Layer

## Why

`platformio.ini` declares `ESP32_PWM_Fusion=symlink:///home/sun/Filelink/Arduino/libraries/ESP32_PWM_Fusion`, but that directory no longer contains the Dlloydev "ESP32 ESP32S2 AnalogWrite" library its name suggests — it contains **ESP32_EasyKit v1.1.0**, a rewritten MCPWM + LEDC library (EasyServo/EasyMotor/EasyLED). The name is a lie, and meanwhile the project's own code (`common/HardwareInit.h`) bypasses every library and drives raw LEDC calls, which can exceed the ESP32-S3's 8 LEDC channels (9 needed with a fully-loaded config: 2 motor + 1 servo + 6 lights). EasyKit is the only PWM library in play written for the project's Arduino 3.x / ESP-IDF 5.x toolchain (`driver/mcpwm_prelude.h`); the `references/` libraries all use the removed legacy `driver/mcpwm.h` API.

## What Changes

- **Vendor** the PWM library into the repo as **`lib/ESP32_EasyKit`** and rename the dependency from `ESP32_PWM_Fusion` to `ESP32_EasyKit` in `platformio.ini`, removing the machine-local symlink so the repo carries the library and its fixes
- **BREAKING**: Replace raw `ledcAttach`/`ledcWrite` calls in `common/HardwareInit.h` with EasyKit classes — `EasyMotor` (DRIVER_2PWM and DRIVER_1PWM_1DIR), `EasyServo` (steering + ESC PPM), `EasyLED` (6 light channels) — keeping the existing `setMotor`/`setServo`/`setLight` public API so `VehicleController` and `main.cpp` are untouched
- **Fix EasyKit bugs surfaced during analysis**: EasyServo's 10× pulse-width scaling error (timer at 10 MHz, µs written directly to the comparator) and `setFrequency()` not reconfiguring the timer
- Update `openspec/config.yaml` and any docs that still say "ESP32_PWM_Fusion"
- Document in `references/` which libraries are unbuildable on this toolchain (legacy MCPWM API) and **remove** the obsolete pristine `ESP32_PWM_Fusion` copy

## Capabilities

### New Capabilities
- `pwm-easykit-integration`: EasyKit as the project's PWM layer — dependency renamed to `ESP32_EasyKit`, HardwareInit routed through EasyMotor/EasyServo/EasyLED, MCPWM/LEDC channel budget enforced, and the servo pulse-timing bugs fixed

### Modified Capabilities
- None (existing specs `unified-firmware-entrypoint`, `littlefs-config-consolidation`, `sound-engine-config` do not change at the requirement level)

## Impact

- **`platformio.ini`**: remove `ESP32_PWM_Fusion=symlink://...`; reference the vendored `lib/ESP32_EasyKit` (local lib_deps entry and/or `-I lib/ESP32_EasyKit/src`, mirroring `lib/SoundEngine`)
- **`lib/ESP32_EasyKit`** (new): vendored EasyKit source copied from `/home/sun/Filelink/Arduino/libraries/` — carries the servo pulse-scaling and `setFrequency` fixes; the machine-local copy is no longer required to build
- **`common/HardwareInit.h`**: swap raw LEDC calls for EasyKit objects; keep public API (`setMotor(int16_t)`, `setServo(int16_t)`, `setLight(pin, pct)`) stable; `stopAll()`/hot-reload now calls `end()`/`detach()`
- **`openspec/config.yaml`**: context line "ESP32_PWM_Fusion (local) for PWM control" → "ESP32_EasyKit (local)"
- **`lib/SoundEngine`**, **`src/main.cpp`**, **`src/RADIOKIT.h`**, **`common/VehicleController.h`**: no changes (they call the stable `HardwareInit` API)
- **`references/`**: add README note marking `ESP32Servo`, `ESP32MCServo`, `ESP32_MCPWM` as legacy-API / unbuildable on Arduino 3.x; **delete** the obsolete pristine `ESP32_PWM_Fusion` copy
- **Dependency**: ESP32_EasyKit v1.1.0 (MIT, `rambros3d/ESP32_EasyKit`) — vendored at `lib/ESP32_EasyKit`; the mislabeled machine-local copy is no longer referenced by the build
