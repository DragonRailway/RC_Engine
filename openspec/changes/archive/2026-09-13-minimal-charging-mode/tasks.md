## 1. Hardware & Power Latch Fixes

- [x] 1.1 Fix `HardwareInit::powerOff()`: replace flawed `esp_deep_sleep(0)` with clean halting / deep-sleep start to prevent 0µs timer reset loops on USB power.
- [x] 1.2 Add USB presence and charging state detection in `HardwareInit` (`isUsbPowered()`, `isCharging()`).
- [x] 1.3 Expose boot reason in `HardwareInit` (`isUsbBoot()`) when power is latched with `POWER_BUTTON == LOW`.

## 2. Charging State Machine & Indicator Logic

- [x] 2.1 Update `VehicleController` state tracking to support `CHARGING_ACTIVE` and `CHARGING_COMPLETE` modes.
- [x] 2.2 Implement heartbeat double-pulse animation (120ms ON, 120ms OFF, 120ms ON, 640ms OFF @ 100% max brightness) during `CHARGING_ACTIVE`.
- [x] 2.3 Implement solid 20% max brightness indicator output when transitioned to `CHARGING_COMPLETE`.
- [x] 2.4 Inhibit disconnect auto-power-off timer while in `CHARGING_ACTIVE` or `CHARGING_COMPLETE`.
- [x] 2.5 Add USB unplug detection in charging mode to power off cleanly if unplugged before switching ON.

## 3. Minimal Boot & Power Button "Switch On"

- [x] 3.1 Update `src/main.cpp` to defer `initRadioKit()` (BLE modem) and `engine.begin()` (audio synthesis) when booted via USB into charging mode.
- [x] 3.2 Add power button switch-on handler in `src/main.cpp` / `VehicleController` to initialize deferred subsystems and transition to active driving mode upon button press.

## 4. Verification & Testing

- [x] 4.1 Run host unit test suite (`pio test`) to ensure no regressions in vehicle control logic.
- [x] 4.2 Build firmware for `TRACKLINK_V2` and `MIKRO_V2` environments (`pio run`).
- [x] 4.3 Upload to connected board and monitor serial telemetry: verify no bootloops occur, charging indicator functions, and button press switches the car ON.
