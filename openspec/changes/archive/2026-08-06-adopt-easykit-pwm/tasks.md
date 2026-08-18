# Tasks: Adopt ESP32_EasyKit as the PWM Layer

## 1. Vendor Dependency

- [x] 1.1 Vendor EasyKit into the repo: copy `/home/sun/Filelink/Arduino/libraries/ESP32_PWM_Fusion` → `lib/ESP32_EasyKit` (src/ + metadata + LICENSE; exclude build artifacts and .git)
- [x] 1.2 Update `platformio.ini`: remove `ESP32_PWM_Fusion=symlink://...`; reference `lib/ESP32_EasyKit` via local lib_deps entry and/or `-I lib/ESP32_EasyKit/src` (mirroring `lib/SoundEngine`)
- [x] 1.3 Update `openspec/config.yaml` context line from "ESP32_PWM_Fusion (local) for PWM control" to "ESP32_EasyKit (local)"
- [x] 1.4 Update any remaining docs/AGENTS.md references to the old name
- [x] 1.5 Verify `pio run -e TRACKLINK_V3` still builds with the vendored dependency before refactoring

## 2. Fix EasyKit Library Bugs

- [x] 2.1 In `lib/ESP32_EasyKit/src/EasyServo.cpp` `attach()`, change `timer_cfg.resolution_hz` from 10,000,000 to 1,000,000 (1 µs/tick) so µs comparator writes are correct
- [x] 2.2 Reconfigure the MCPWM timer period in `EasyServo::setFrequency()` in `lib/ESP32_EasyKit/src/EasyServo.cpp` (call the same timer-config path used in `attach()`) so frequency changes apply at runtime
- [x] 2.3 Sanity-check EasyServo `attach()` and `_applyDuty()` comparator values are now µs-accurate (1500 µs pulse at 50 Hz = 20 ms period, 1500-tick compare)

## 3. HardwareInit Migration

- [x] 3.1 Add EasyKit statics to `HardwareInit`: one `EasyMotor`, one `EasyServo` (steering), one `EasyServo` (ESC, reused when motor type is ESC), and up to six `EasyLED`
- [x] 3.2 Rewrite `initDriveMotor()`: HBRIDGE_A → `DRIVER_2PWM`, HBRIDGE_B → `DRIVER_1PWM_1DIR`, ESC → EasyServo PPM (1000–2000 µs @ 50 Hz); apply enable pin, frequency (24 kHz default), and min/max duty window from config
- [x] 3.3 Rewrite `initSteeringServo()`: EasyServo attach with endpoints from config (left/right/center µs) at configured frequency
- [x] 3.4 Rewrite `initLights()`: `EasyLED::begin(pin, {freq: 5000, resolution: Bits10})` per configured light; keep head/tail/brake/turn/reversing pin tracking
- [x] 3.5 Keep `setMotor(int16_t)`, `setServo(int16_t)`, `setLight(pin, pct)` public API behavior identical (same direction handling, duty windows, endpoint mapping, brightness scaling)
- [x] 3.6 Rewrite `stopAll()` to `end()`/`detach()` all EasyKit objects so `hotReload()` can re-attach cleanly
- [x] 3.7 Remove all raw `ledcAttach`/`ledcWrite`/`ledcDetach` calls from `HardwareInit.h`

## 4. References Documentation

- [x] 4.1 Delete `references/ESP32_PWM_Fusion` (pristine Dlloydev copy) — **done**, directory removed from disk (was untracked, so no `git rm` needed)
- [x] 4.2 Add `references/README.md` documenting which libraries build on this toolchain (EasyKit: yes, `mcpwm_prelude.h`; ESP32Servo/ESP32MCServo/ESP32_MCPWM: legacy `driver/mcpwm.h`, unbuildable without migration)

## 5. Validation

- [x] 5.1 Build both environments: `pio run -e TRACKLINK_V3` and `pio run -e MIKRO_V2` — both SUCCESS
- [x] 5.2 Smoke-test on hardware (or logic analyzer): motor forward/reverse on both bridge types, servo sweep ±endpoints, ESC neutral → throttle, all six lights at various brightness — **verified on Mikro V2**: servo sweep, all 6 lights, motor, reverse, page switching, horn via RadioKit protocol (25 ACK frames, 0 errors)
- [x] 5.3 Exercise hot-reload: apply a second hardware config at runtime and confirm re-attach without channel exhaustion — **verified live**: wire-uploaded config (VOLUME=70) over RK FS protocol → watcher fired `Reloading Configs` → `HardwareInit: Hot-reloading...` → `Configs reloaded OK`, persisted across reboot
- [x] 5.4 Verify LEDC usage stays ≤ 8 channels and MCPWM operators stay ≤ 12 for a fully-loaded config — verified in code: 3 MCPWM slots (motor + steering + ESC) ≤ 12, 6 LEDC channels ≤ 8; no raw `ledcAttach/Write/Detach` remain in HardwareInit
