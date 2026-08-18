## Why

The current configuration system (`hardware-config.json` and `vehicle-config.json`) uses inconsistent `UPPERCASE` key casing, lacks explicit drivetrain topology auto-deduction (skid-steer vs. Ackermann), and relies on flat hyphenated sound file names (`/sounds/ScaniaV8-idle.json`). This leads to duplicate sound files across vehicle profiles, cumbersome asset maintenance on LittleFS, and inconsistent JSON formatting.

Standardizing key casing to `lower_snake_case`, adding automatic drivetrain deduction in `hardware-config.json`, and reorganizing `/sounds/` into a 3-tier hierarchical preset structure (`vehicles/`, `presets/`, `generic/`) will significantly simplify UX, eliminate asset duplication, and improve system maintainability.

## What Changes

- **Standardized Key Casing**: Standardize all JSON keys in `hardware-config.json` and `vehicle-config.json` to `lower_snake_case`.
- **Implicit Drivetrain Deductive Topology**: Encapsulate drivetrain hardware under `"drivetrain"`, automatically detecting Skid-Steer (`left_motor` + `right_motor`) vs. Ackermann (`drive_motor` + `steering_servo`).
- **Explicit Sound Set Association**: Add explicit `"sound_set"` and optional `"preset"` fields to `vehicle-config.json`.
- **Hierarchical Sound Asset Organization**: Reorganize LittleFS `/sounds/` directory into `vehicles/`, `presets/`, and `generic/` subdirectories with a 3-tier fallback resolution pipeline.
- **Config Parser & Loader Update**: Update `ConfigParser.h` and `ConfigLoader.h` to parse `lower_snake_case` keys, deduct drivetrain mode, and implement 3-tier sound resolution.

## Capabilities

### New Capabilities
- None

### Modified Capabilities
- `config-filesystem-management`: Update configuration JSON key schema (`lower_snake_case`), add deductive drivetrain layout support (`skid_steer` vs `ackermann`), and update LittleFS file paths for sound presets.
- `sound-engine-config`: Add 3-tier sound resolution (`vehicles/` -> `presets/` -> `generic/`) and support `sound_set` & `preset` fields in `vehicle-config.json`.

## Impact

- `common/Config.h`, `common/ConfigParser.h`, `common/ConfigLoader.h`, `common/HardwareInit.h`, `common/VehicleController.h` updated for `lower_snake_case` and deductive drivetrain topology.
- Default configuration files in `data/` and `src/example_config/` updated to the new schema.
- `/sounds/` and `/data/sounds/` directory assets reorganized into `vehicles/`, `presets/`, and `generic/` subfolders.
- RadioKit API & FS reload compatibility maintained.
