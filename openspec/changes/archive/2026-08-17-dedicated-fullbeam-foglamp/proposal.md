## Why

Vehicles with realistic lighting setups require physical independence between low beam, high beam (full beam), fog lamps, and locomotive ditch lights. Previously, full beam was implemented solely via PWM brightness stepping on the low beam pin, and truck fog lamps were borrowing locomotive ditch light pins (`ditch_light`). Introducing dedicated pin definitions for `full_beam` and `fog_lamp` allows vehicles to wire distinct bulbs/LEDs for each physical lighting group and eliminates conflation with locomotive alternating ditch lights.

## What Changes

- Add dedicated `full_beam` (alias `high_beam`) lighting configuration under `lights` in `HardwareConfig`, allowing an independent physical pin for high/full beam lights.
- Add dedicated `fog_lamp` (alias `fog_light`) lighting configuration under `lights` in `HardwareConfig`, supporting dedicated physical wiring for trucks/cars independent of locomotive ditch lights.
- Update `common/Config.h` and `common/ConfigParser.h` to parse and initialize `full_beam` and `fog_lamp`.
- Update `configs/schemas/hardware_config.schema.json` to formally validate `full_beam` and `fog_lamp`.
- Update `common/VehicleController.h` so that Low Beam (Bit 0) energizes `head_light`, High Beam (Bit 1) energizes `full_beam`, and Fog Lamp (Bit 2) energizes `fog_lamp`.
- Update `GUIDE/HARDWARE_CONFIG.md` to document the dedicated pin configurations for `head_light`, `full_beam`, `fog_lamp`, and `ditch_light`.

## Capabilities

### Modified Capabilities
- `advanced-lighting-automation`: Update lighting requirements to specify dedicated physical pin outputs for `full_beam` and `fog_lamp` alongside `head_light` and `ditch_light`.
- `config-reference-guide`: Document `lights.full_beam` and `lights.fog_lamp` in the hardware configuration guide.

## Impact

- **Hardware configs**: Existing configs without `full_beam` or `fog_lamp` continue to work with graceful unconfigured fallbacks; configs specifying `full_beam` and `fog_lamp` get independent physical pin assignments.
- **Firmware**: `common/Config.h`, `common/ConfigParser.h`, `common/VehicleController.h`, `common/HardwareInit.h`.
- **Validation**: `configs/schemas/hardware_config.schema.json` and `scripts/validate_configs.py`.
- **Documentation**: `GUIDE/HARDWARE_CONFIG.md`.
