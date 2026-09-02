## MODIFIED Requirements

### Requirement: Lower snake_case JSON schema
The firmware SHALL parse hardware configuration (dynamically discovered via `hardware-*.json`) and vehicle configuration (dynamically discovered via `vehicle-*.json` / `vehicle.json` in root or vehicle config subdirectories) using `lower_snake_case` JSON keys. The hardware schema SHALL additionally accept an optional `"animation"` block (easing speed/strength, fade duration) whose absence falls back to firmware defaults.

#### Scenario: Parse lower_snake_case hardware config
- **WHEN** the dynamically discovered hardware config containing `sound`, `drivetrain`, `lights`, and `battery` keys in `lower_snake_case` is loaded
- **THEN** `ConfigParser` successfully populates `HardwareConfig` with all specified parameters

#### Scenario: Parse lower_snake_case vehicle config
- **WHEN** the dynamically discovered vehicle config containing `vehicle`, `engine`, `transmission`, and `sound_volumes` keys in `lower_snake_case` is loaded
- **THEN** `ConfigParser` successfully populates `RcEngineSound::Config` with all specified parameters

#### Scenario: Parse optional animation block
- **WHEN** the hardware config contains an `"animation"` block with `easing_speed_deg_s`, `easing_k_in`, `easing_k_out`, and `fade_duration_ms`
- **THEN** `ConfigParser` populates the corresponding `HardwareConfig` animation fields

#### Scenario: Animation block absent uses defaults
- **WHEN** the hardware config omits the `"animation"` block
- **THEN** `ConfigParser` leaves the animation fields at their firmware defaults and loading still succeeds

## ADDED Requirements

### Requirement: Dynamic Configuration Discovery
The firmware SHALL dynamically discover hardware and vehicle configuration files on LittleFS without requiring hardcoded static filenames or compile-time file path defines:
1. **Hardware Configuration**: `ConfigParser::findHardwareConfig()` SHALL scan the LittleFS root directory (`/`) and return the path of the first file starting with `hardware-` and ending with `.json`.
2. **Vehicle Configuration**: `ConfigParser::findVehicleConfig()` SHALL search for the first vehicle configuration matching:
   - Root `/`: `vehicle.json` or any file starting with `vehicle-` and ending with `.json`.
   - Subdirectories `/vehicle_configs/` and `/vehicle_config/`: `<dir>/vehicle.json`, `<dir>/vehicle-*.json`, or nested vehicle bundle JSON files.

#### Scenario: Dynamic hardware config discovery with board variant name
- **WHEN** LittleFS contains `/hardware-MIKRO_V2-truck.json`
- **THEN** `ConfigParser::findHardwareConfig()` returns `"/hardware-MIKRO_V2-truck.json"` and successfully loads the hardware configuration

#### Scenario: Dynamic vehicle config discovery in root
- **WHEN** LittleFS contains `/vehicle-ScaniaV8.json`
- **THEN** `ConfigParser::findVehicleConfig()` returns `"/vehicle-ScaniaV8.json"` and successfully loads the vehicle configuration

#### Scenario: Dynamic vehicle config discovery in subdirectory bundle
- **WHEN** LittleFS contains `/vehicle_configs/ScaniaV8/vehicle.json` and no vehicle config in root
- **THEN** `ConfigParser::findVehicleConfig()` returns `"/vehicle_configs/ScaniaV8/vehicle.json"` and successfully loads the vehicle configuration
