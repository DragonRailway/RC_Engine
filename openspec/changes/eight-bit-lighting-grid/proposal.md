## Why

The RadioKit lighting control widget uses an 8-item MultipleSelect widget (`truck_light` / `loco_light`) that controls on-board lighting channels via an 8-bit mask (bits 0 to 7). Previously, several lighting functions (such as beacon lights, work lights, and cab lights) were either unmapped, shared with locomotive ditch lights, or lacked a formal 8-bit multi-button dispatch architecture. Furthermore, unconfigured lighting channels remained active and clickable in the UI rather than being dynamically disabled based on the board's hardware configuration.

Standardizing the 8-bit light control mapping across trucks and locomotives and dynamically disabling unconfigured light buttons gives realistic scale modeling control and intuitive UI feedback.

## What Changes

- Map the 8-bit MultipleSelect control widget mask to distinct lighting functions across vehicle types:
  - **Bit 0 (`0x01`)**: `lights.head_light` (Head Light)
  - **Bit 1 (`0x02`)**: `lights.full_beam` (High / Full Beam)
  - **Bit 2 (`0x04`)**: `lights.fog_lamp` (Fog Lamp)
  - **Bit 3 (`0x08`)**: `hazard light` (Truck) / `lights.ditch_light` (Locomotive)
  - **Bit 4 (`0x10`)**: `lights.beacon` (Roof Strobe / Rotary Beacon Light)
  - **Bit 5 (`0x20`)**: `lights.cab_light` (Interior Cabin Light)
  - **Bit 6 (`0x40`)**: `lights.work_light` (Truck) / `lights.step_light` (Locomotive)
  - **Bit 7 (`0x80`)**: `lights.aux_light` (Auxiliary Light)
- Note: Turn signals (`turn_light.left` and `turn_light.right`) remain on their independent dedicated toggle buttons on the UI (`left_indicator` and `right_indicator`).
- Introduce `lights.beacon` (with strobe/rotary pattern runner support) and `lights.work_light` into `HardwareConfig`, `ConfigParser`, and hardware JSON schemas.
- Update `common/VehicleController.h` to dispatch all 8 bits to their respective lighting channels and execute beacon patterns.
- Implement dynamic unconfigured button disabling: the firmware computes the configured light mask and syncs it with RadioKit UI so unconfigured buttons are disabled/locked in the UI.
- Update UI design (`docs/radiokit-rc-ui-design.json`) and generated RadioKit header (`src/RADIOKIT.h`) with 8 items.
- Update `GUIDE/HARDWARE_CONFIG.md` with the complete 8-bit mapping and new lighting keys.

## Capabilities

### Modified Capabilities
- `advanced-lighting-automation`: Update lighting automation requirements to specify the 8-bit control mapping, beacon light pattern runner, work light control, and dynamic UI item disabling.
- `config-reference-guide`: Document `lights.beacon`, `lights.work_light`, and the standardized 8-bit lighting grid in `GUIDE/HARDWARE_CONFIG.md`.

## Impact

- **Firmware**: `common/Config.h`, `common/ConfigParser.h`, `common/HardwareInit.h`, `common/VehicleController.h`, `src/RADIOKIT.h`.
- **UI Design**: `docs/radiokit-rc-ui-design.json`.
- **Schema & Configs**: `configs/schemas/hardware_config.schema.json`, `configs/hardware_configs/hardware-MIKRO_V2-truck.json`.
- **Documentation**: `GUIDE/HARDWARE_CONFIG.md`.
- **Tests**: `test/host_vc/host_vc_driver.cpp`, `scripts/verify_lights_jtag.py`.
