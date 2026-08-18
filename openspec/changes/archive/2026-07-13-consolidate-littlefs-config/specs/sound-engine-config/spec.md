## MODIFIED Requirements

### Requirement: RcEngineSound Config struct
The `RcEngineSound::Config` struct SHALL include a `char name[32]` field for the vehicle name, in addition to all existing engine, sound, transmission, features, and loopPoints fields.

#### Scenario: Config contains vehicle name
- **WHEN** `RcEngineSound::Config` is populated from JSON
- **THEN** `config.name` contains the vehicle name string (max 31 chars + null terminator)

#### Scenario: Config retains all existing fields
- **WHEN** `RcEngineSound::Config` is populated from JSON
- **THEN** all existing fields (engine.acc, engine.dec, sound.start, sound.idle, transmission.type, features.*, loopPoints.*) are populated as before

## ADDED Requirements

### Requirement: VehicleType enum
The sound engine library SHALL define a `VehicleType` enum with values: `TRUCK`, `EXCAVATOR`, `LOCOMOTIVE`, `CAR`, `TANK`, and `UNKNOWN`.

#### Scenario: Enum available to app code
- **WHEN** the sound engine header is included
- **THEN** `VehicleType` enum is accessible and usable in app code

### Requirement: Compile-time vehicle type
The vehicle type SHALL be set at compile time via a `VEHICLE_TYPE` build flag that maps to the `VehicleType` enum.

#### Scenario: Build flag defines type
- **WHEN** platformio.ini specifies `-D VEHICLE_TYPE_TRUCK=0`
- **THEN** the app code can reference `VEHICLE_TYPE_TRUCK` as an integer matching the enum

#### Scenario: Library uses compile-time type
- **WHEN** the sound engine library needs vehicle type information
- **THEN** it reads the compile-time constant, not a runtime config value
