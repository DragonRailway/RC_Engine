## 1. Engine Start Control Bit

- [x] 1.1 Add the latched `engine_start` toggle widget to `src/RADIOKIT.h` (Truck page) and wire `VehicleController::update()` to read `engine_start.rk.state` — ON starts (OFF→STARTING→RUNNING), OFF stops (RUNNING→OFF, cancels mid-crank)
- [x] 1.2 Keep the reversing light on Item E (bit 4) in `applyLightsWithAutomation()` — no Item F needed since start no longer uses the bitmask
- [x] 1.3 Verify the battery-cutoff light override call passes a zeroed bitmask (hazards-only alarm state) to `applyLightsWithAutomation`

## 2. Strict Engine Start

- [x] 2.1 Remove the `throttlePct > 10` auto-start condition so the Engine Power toggle is the sole start trigger
- [x] 2.2 Confirm motor drive commands remain gated to the RUNNING state only (OFF/STARTING keep motor at 0)

## 3. Config-Driven Voltage Calibration

- [x] 3.1 Update `ConfigParser.h` `voltage_scale`/`voltage_offset` parse defaults from `1.0f`/`0.0f` to `VSCALE`/`VOFFSET` macros with an `#ifndef` fallback to `1.0f`/`0.0f`
- [x] 3.2 Replace `VSCALE`/`VOFFSET` macro usage in `VehicleController` battery math (boot cell detection, runtime cutoff monitor, telemetry percent) with `s_hw->telemetry.vScale`/`s_hw->telemetry.vOffset`
- [x] 3.3 Update `data/hardware-config.json` telemetry block to `voltage_scale: 1.8`, `voltage_offset: -0.2` (mirrors the previous macro calibration)

## 4. Documentation & UI Contract

- [x] 4.1 Update comments in `common/HardwareInit.h` noting Aux Servo 2 (S3) is available on MIKRO_V2 only; TRACKLINK_V3 has no S3/S4
- [ ] 4.2 (user-managed) Add the `engine_start` toggle button (type `button`, mode `toggle`, name `engine_start`) to `docs/radiokit-rc-ui-design.json` and regenerate `src/RADIOKIT.h` so the firmware's widget declaration matches the design

## 5. Validation

- [x] 5.1 Build with `pio run` (default TRACKLINK_V3 env) and confirm no compile errors
- [x] 5.2 Build the MIKRO_V2 env if configured, confirming the calibration defaults guard compiles
- [x] 5.3 Review the diff against the delta specs (each spec scenario has a matching behavior in code)
