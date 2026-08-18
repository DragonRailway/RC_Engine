# Tasks: Board Power & Charging Visual Indicators

## Section 1: Schema & Config Parser

- [x] 1.1 Update `configs/schemas/hardware_config.schema.json`: add `"charging"` section (`hardware`, `mode`) and `"hardware"` property in `"power"` object
- [x] 1.2 Update `common/Config.h`: add `Charging` struct (`pin`, `mode`, `configured`) and `indicatorPin` field in `Power` struct
- [x] 1.3 Update `common/ConfigParser.h`: parse `"charging"` section and `"power.hardware"` indicator with light alias resolution
- [x] 1.4 Update shipped hardware configs (`hardware-*.json`) to include `"charging"` configuration block

## Section 2: Active Visual Feedback & Charging Indicator

- [x] 2.1 Update `HardwareInit::updatePowerButton()`: implement rapid 200ms blink feedback on `power.indicatorPin` during button hold
- [x] 2.2 Update `VehicleController::update()` / `HardwareInit`: implement charging indicator animation (`solid`, `blink`, `pulse`) when in `CHARGING` state
- [x] 2.3 Integrate `power.indicatorPin` into 10-second disconnect warning phase

## Section 3: Documentation & Config Validation

- [x] 3.1 Update `GUIDE/HARDWARE_CONFIG.md` sections 7 and 8 to document the `"charging"` block and `"power.hardware"` indicator
- [x] 3.2 Run `python3 scripts/validate_configs.py` to confirm all hardware configs pass schema validation

## Section 4: Build & Unit Test Verification

- [x] 4.1 Build firmware environments (`MIKRO_V2`, `TRACKLINK_V3`) via `pio run`
- [x] 4.2 Update `test/host_vc/host_vc_driver.cpp` (Test 13) to verify power button hold rapid blink feedback, charging indicator animation modes, and light alias resolution
