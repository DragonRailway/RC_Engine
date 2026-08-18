## 1. Config Structs & Parser

- [x] 1.1 Add `fullBeam` (Light) and `fogLamp` (Light) to `HardwareConfig::Lights` in `common/Config.h`.
- [x] 1.2 Implement `parseLights` support for `full_beam` (alias `high_beam`) and `fog_lamp` (alias `fog_light`) in `common/ConfigParser.h`.
- [x] 1.3 Add pin initialization and shutdown support for `fullBeam` and `fogLamp` in `common/HardwareInit.h`.

## 2. Vehicle Controller & Lighting Control Loop

- [x] 2.1 Update `VehicleController::applyLightsWithAutomation()` to drive `full_beam` pin on High Beam (Bit 1) with fallback to `head_light` if unconfigured.
- [x] 2.2 Update `VehicleController::applyLightsWithAutomation()` to drive `fog_lamp` pin on Fog Lamp (Bit 2), decoupled from `ditch_light`.
- [x] 2.3 Ensure locomotive `ditch_light` alternating flashing logic is preserved exclusively for locomotive operations.

## 3. Schema Validation & Configs

- [x] 3.1 Update `configs/schemas/hardware_config.schema.json` to include `full_beam` and `fog_lamp` schemas.
- [x] 3.2 Update `configs/hardware_configs/hardware-MIKRO_V2-truck.json` with distinct pin assignments for `head_light` (L1), `full_beam` (L6), `fog_lamp` (L7).
- [x] 3.3 Validate all configs using `scripts/validate_configs.py`.

## 4. Documentation & Verification

- [x] 4.1 Update `GUIDE/HARDWARE_CONFIG.md` Section 4 to document `head_light`, `full_beam`, `fog_lamp`, and `ditch_light`.
- [x] 4.2 Update host harness tests in `test/host_vc/host_vc_driver.cpp` to verify dedicated full beam and fog lamp outputs.
- [x] 4.3 Run host tests (`host_vc_harness`), build firmware for `MIKRO_V2`, and execute `scripts/verify_mikro_e2e.py` and `scripts/verify_remote_api_widgets.py`.
