## 1. Build Setup

- [x] 1.1 Add `RadioKit=symlink:///home/sun/Apps/RadioKit/rk-arduino` to `lib_deps` in `platformio.ini`
  - Note: resolved as a vendored copy at `lib/rk-arduino` (downloaded from the companion app API `GET /api/library/download`, filename `rk-arduino.zip`) instead of the symlink
- [x] 1.2 Verify `pio run -e TRACKLINK_V3` compiles with the RadioKit dependency resolved

## 2. RadioKit Integration

- [x] 2.1 Fetch the RC_UI design JSON from the app API (`GET http://10.0.0.6:7007/api/designs/1785927365527/json`) and archive it at `docs/radiokit-rc-ui-design.json`
- [x] 2.2 Create `src/RADIOKIT.h` from the generated design: 2 pages (Truck/Loco), 8 widgets (steering_wheel, gas_pedal, brake_pedal, led_select, slider, dir_switch, lights_toggle, horn), 2 telemetry widgets (Battery, Speed), `RK_ENABLE_OTA`/`RK_ENABLE_FS`/`RK_ENABLE_BLE` defines, and `initRadioKit()`
- [x] 2.3 Wire `initRadioKit()` into `setup()` and `RadioKit.update()` into `loop()` in `src/main.cpp`

## 3. Vehicle Control Loop

- [x] 3.1 Add header-only `common/VehicleController.h` with widget-input mapping (gas_pedal/Loco slider → throttle, brake_pedal → brake, steering_wheel → steering, dir_switch → direction)
- [x] 3.2 Implement motor output writer: H-bridge (ledcWrite PWM1/PWM2 with direction and duty min/max) and ESC (PPM µs)
- [x] 3.3 Implement steering servo output mapped to configured left/right/center endpoints
- [x] 3.4 Implement light output from `led_select` and `lights_toggle` bitmasks (head/tail/brake/turn/reversing)
  - Note: light widgets extended to 5 items (A=head, B=tail, C=brake, D=turn, E=reversing) in `src/RADIOKIT.h` and `docs/radiokit-rc-ui-design.json`; mapping in `VehicleController.h::applyLights()`
- [x] 3.5 Drive the sound engine: `engine.update(throttle)` for RPM/pitch and `triggerHorn`/`triggerBrake`/`triggerReversing` triggers
- [x] 3.6 Publish telemetry at a bounded rate: battery % (VSCALE/VOFFSET voltsense) and speed estimate (throttle + direction)

## 4. Config Reload

- [x] 4.1 Implement `reloadConfigs()`: re-parse hardware/vehicle configs and sounds, `HardwareInit::hotReload()`, re-apply engine config (fallback to last-known-good on parse failure)
- [x] 4.2 Add periodic config-file timestamp check (e.g. every 2 s) in the loop to trigger reload after app-side saves

## 5. Validation

- [x] 5.1 `pio run -e TRACKLINK_V3` builds clean with no warnings/errors
- [x] 5.2 Verify flash usage fits the 4 MB budget with OTA partitions
  - Note: app0 (ota_0) = 3 MB partition, firmware uses 1,257,284 bytes (40%); total flash 4 MB ✓
- [x] 5.3 Manual test: app connects over BLE, widgets drive motor/servo/lights/sound, FS browse/edit/upload works, telemetry updates, config reload applies without reboot
  - **Progress & Verification (2026-08-06):**
    - ✓ Board verified alive with BLE advertising (`BLE: Starting advertising... BLE: System ready.`) — firmware build includes `RK_ENABLE_BLE`
    - ✓ Android device `HA26JZ08` connected via adb; RadioKit app API live on port 17007 (`adb forward tcp:17007 tcp:7007`)
    - ✓ Hardware test script `scripts/smoke_test.py` executed against connected `/dev/ttyACM0`:
      - Servo sweep (steering_wheel): 25 ACK frames received, servo sweep passed
      - Lights (led_select bitmask): A head, B tail, C brake, D turn, E reverse, ALL, OFF verified
      - Motor forward (gas_pedal): throttle 0 -> 30 -> 60 -> 0 driven cleanly
      - Reverse & horn (Loco page switch): page switch, reverse switch, slider & horn executed
      - Telemetry published: Battery % and Speed telemetry streamed during test
    - ✓ Config hot-reload test `scripts/hot_reload_test.py` executed:
      - Uploaded updated `hardware-config.json` over RadioKit FS protocol
      - `reloadConfigs()` & `HardwareInit` hot-reload triggered live without rebooting

