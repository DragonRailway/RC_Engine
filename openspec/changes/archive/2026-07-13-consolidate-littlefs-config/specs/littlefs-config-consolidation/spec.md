## ADDED Requirements

### Requirement: ConfigParser handles all JSON parsing
ConfigParser SHALL be the single authority for parsing all JSON config files from LittleFS, including hardware config, vehicle config, and sound files.

#### Scenario: Hardware config loaded
- **WHEN** `ConfigParser::loadHardwareConfig("/hardware-config.json", hwConfig)` is called
- **THEN** the HardwareConfig struct is populated with pin assignments, motor settings, servo endpoints, and light configurations

#### Scenario: Vehicle config loaded
- **WHEN** `ConfigParser::loadVehicleConfig("/vehicle-ScaniaV8.json", vehicleConfig)` is called
- **THEN** the RcEngineSound::Config struct is populated with engine parameters, transmission settings, sound volumes, features, loop points, and mix weights

#### Scenario: Vehicle config missing
- **WHEN** `ConfigParser::loadVehicleConfig()` is called with a nonexistent file
- **THEN** the function returns false and the config struct retains default values

### Requirement: Sound files loaded by ConfigParser
ConfigParser SHALL load sound files from `/sounds/` directory using vehicle-specific naming convention.

#### Scenario: Vehicle-specific sounds loaded
- **WHEN** `ConfigParser::loadSounds("ScaniaV8", soundData)` is called
- **THEN** files matching `/sounds/ScaniaV8-*.json` are loaded into SoundData slots

#### Scenario: Fallback to generic sounds
- **WHEN** a vehicle-specific sound file is not found
- **THEN** ConfigParser falls back to the generic sound file name (e.g., `idle-ScaniaV8.json`)

#### Scenario: Sound file invalid
- **WHEN** a sound JSON file has missing or corrupt sample data
- **THEN** that sound slot is left empty and parsing continues with remaining sounds

### Requirement: VehicleConfig struct deleted
The `VehicleConfig` struct in `Config.h` SHALL be removed. `RcEngineSound::Config` is the sole vehicle config struct.

#### Scenario: No VehicleConfig references
- **WHEN** the codebase is compiled after the change
- **THEN** no references to `VehicleConfig` exist in any source file

### Requirement: VehicleProfile simplified
VehicleProfile SHALL be reduced to a thin container or deleted entirely, with its parsing logic moved to ConfigParser.

#### Scenario: VehicleProfile no longer parses JSON
- **WHEN** VehicleProfile exists after the change
- **THEN** it contains no `deserializeJson` calls or JSON file reading logic

### Requirement: Vehicle name in config
RcEngineSound::Config SHALL include a `name[32]` field populated from the JSON `"VEHICLE": { "NAME": "..." }` field.

#### Scenario: Name parsed from JSON
- **WHEN** vehicle config JSON contains `"VEHICLE": { "NAME": "Scania V8" }`
- **THEN** `config.name` equals `"Scania V8"`

#### Scenario: Name missing in JSON
- **WHEN** vehicle config JSON has no `VEHICLE.NAME` field
- **THEN** `config.name` equals `"Unknown"`

### Requirement: Vehicle type as build flag
Vehicle type SHALL be a compile-time constant set via build flag, not a runtime JSON value.

#### Scenario: RC_Truck build
- **WHEN** building with `pio run -e RC_Truck`
- **THEN** the `VEHICLE_TYPE` enum is set to `TRUCK`

#### Scenario: RC_Loco build
- **WHEN** building with `pio run -e RC_Loco`
- **THEN** the `VEHICLE_TYPE` enum is set to `LOCOMOTIVE`

#### Scenario: JSON has no TYPE field
- **WHEN** vehicle config JSON is parsed
- **THEN** no `TYPE` field is read or required
