## 1. Config Structs & Parser

- [x] 1.1 Add `beacon` and `workLight` (Light structs) to `HardwareConfig::Lights` in `common/Config.h`.
- [x] 1.2 Update `common/ConfigParser.h` to parse `beacon` and `work_light` (with aliases `work_lamp`) and resolve aliases.
- [x] 1.3 Add pin initialization and shutdown support for `beacon` and `workLight` in `common/HardwareInit.h`.

## 2. Vehicle Controller & Lighting Control Loop

- [x] 2.1 Update `VehicleController::applyLightsWithAutomation()` to dispatch all 8 bits:
  - Bit 0 (`0x01`): `head_light`
  - Bit 1 (`0x02`): `full_beam`
  - Bit 2 (`0x04`): `fog_lamp`
  - Bit 3 (`0x08`): Hazard Light (Truck) / Ditch Light (Loco)
  - Bit 4 (`0x10`): Beacon Light (`beacon`)
  - Bit 5 (`0x20`): Cab Light (`cab_light`)
  - Bit 6 (`0x40`): Work Light (`work_light`, Truck) / Step Light (`step_light`, Loco)
  - Bit 7 (`0x80`): Aux Light (`aux_light`)
- [x] 2.2 Add beacon light strobe / flasher pattern runner in `HardwareInit.h`.
- [x] 2.3 Compute configured lights bitmask in `HardwareInit`/`VehicleController` and synchronize with UI to disable unconfigured buttons.

## 3. UI Design & Schema Validation

- [x] 3.1 Update `docs/radiokit-rc-ui-design.json` and `src/RADIOKIT.h` to configure all 8 items for `truck_light` and `loco_light`.
- [x] 3.2 Update `configs/schemas/hardware_config.schema.json` to formally validate `beacon` and `work_light`.
- [x] 3.3 Validate all configs using `scripts/validate_configs.py`.

## 4. Documentation & Verification

- [x] 4.1 Update `GUIDE/HARDWARE_CONFIG.md` Section 4 with complete 8-bit table and documentation for `beacon` and `work_light`.
- [x] 4.2 Update host harness tests in `test/host_vc/host_vc_driver.cpp` to verify all 8 bits.
- [x] 4.3 Build and flash `MIKRO_V2`, and execute `verify_lights_jtag.py` via Android Remote REST API and live USB JTAG.
