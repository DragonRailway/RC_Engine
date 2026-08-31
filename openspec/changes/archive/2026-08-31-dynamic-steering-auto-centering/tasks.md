## 1. Schema & Configuration

- [x] 1.1 Update `configs/schemas/hardware_config.schema.json` with the `auto_centering` object definition under `drivetrain`
- [x] 1.2 Add `AutoCentering` struct to `HardwareConfig` in `common/Config.h`
- [x] 1.3 Update `ConfigParser.cpp` to parse `drivetrain.auto_centering` and set safe defaults
- [x] 1.4 Validate all existing hardware configurations using `scripts/validate_configs.py`

## 2. Core Physics & Control Implementation

- [x] 2.1 Add touch interaction state tracking (`s_lastSteerTouchMs`, `s_lastSteerInputVal`, `s_currentSteerAngle`) in `common/VehicleController.h` and `VehicleController.cpp`
- [x] 2.2 Implement speed-proportional return-to-center physics calculation in `VehicleController::update()`
- [x] 2.3 Implement reverse gear hold logic when `hold_in_reverse` is enabled
- [x] 2.4 Apply dynamic steering output across Ackermann (servos) and Skid-Steer (differential motors) modes
- [x] 2.5 Synchronize decayed steering value to RadioKit `steering_wheel.rk.value` to update the connected app UI

## 3. Verification & Testing

- [x] 3.1 Run host unit test harnesses (`pio test` / `test/host_vc`) to verify steering decay curves and interaction priority
- [x] 3.2 Verify build compiles cleanly for all target environments (`TRACKLINK_V3`, `MIKRO_V2`)
- [x] 3.3 Test with example hardware config (e.g. `hardware-MIKRO_V2-truck.json`) under stationary, forward motion, and reverse conditions
