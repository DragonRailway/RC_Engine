## 1. Config Struct & Parser Updates

- [x] 1.1 Update `common/ConfigParser.h` to parse `lower_snake_case` keys for `hardware-config.json` and `vehicle-config.json` with fallback support for uppercase keys
- [x] 1.2 Implement deductive drivetrain topology detection in `ConfigParser::parseDrivetrain()` (`skid_steer` vs `ackermann`)
- [x] 1.3 Update `common/Config.h` and `common/HardwareInit.h` to handle dual-motor (`left_motor` + `right_motor`) skid-steer output mapping alongside standard single-drive motor output

## 2. Hierarchical Sound Loader Implementation

- [x] 2.1 Update `ConfigParser::loadSounds()` to implement 3-tier lookup order (`/sounds/vehicles/<sound_set>/<slot>.json` -> `/sounds/presets/<preset>/<slot>.json` -> `/sounds/generic/<slot>.json`)
- [x] 2.2 Update `vehicle-config.json` schema parsing to extract `"sound_set"` and `"preset"` strings

## 3. Asset & File Reorganization

- [x] 3.1 Reorganize LittleFS `/sounds/` and `data/sounds/` directories into `vehicles/`, `presets/`, and `generic/` subfolders
- [x] 3.2 Update `data/hardware-config.json`, `data/vehicle-config.json`, and `data/vehicle-*.json` example files to `lower_snake_case` format

## 4. Verification & Testing

- [x] 4.1 Build firmware cleanly with `pio run -e TRACKLINK_V3`
- [x] 4.2 Run hardware smoke test & config hot-reload test (`scripts/smoke_test.py` & `scripts/hot_reload_test.py`) on connected TRACKLINK_V3 board
