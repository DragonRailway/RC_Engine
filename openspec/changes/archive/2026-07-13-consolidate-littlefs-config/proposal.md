## Why

The current config system has two independent parse paths for the same JSON: `ConfigParser::loadVehicleConfig()` fills a `VehicleConfig` struct (incomplete, ~30 fields), while `VehicleProfile::parseConfig()` fills `RcEngineSound::Config` (complete, ~70 fields). This duplication means adding a config field requires changes in two places, and the two structs can silently drift out of sync. The vehicle type is stored as a string in JSON but is actually a compile-time constant per app (RC_Truck, RC_Loco), adding unnecessary runtime complexity.

## What Changes

- **BREAKING**: Delete `VehicleConfig` struct from `Config.h` — `RcEngineSound::Config` becomes the single vehicle config
- **BREAKING**: Add `name[32]` field to `RcEngineSound::Config` for vehicle name
- Add `VehicleType` enum to the sound engine library (`TRUCK`, `EXCAVATOR`, `LOCOMOTIVE`, etc.), set via compile-time build flag (`-D VEHICLE_TYPE_TRUCK`)
- Consolidate all JSON parsing into `ConfigParser`: absorb `VehicleProfile::parseConfig()` and sound loading logic
- Simplify `VehicleProfile` to a thin container or delete it entirely
- Remove vehicle type from JSON config files (type is a build-time constant)
- Keep `HardwareConfig` and `ConfigParser::loadHardwareConfig()` as-is

## Capabilities

### New Capabilities
- `littlefs-config-consolidation`: Single-parse config system where ConfigParser owns all JSON parsing and RcEngineSound::Config is the canonical vehicle config struct

### Modified Capabilities
- `sound-engine-config`: RcEngineSound::Config gains name field and VehicleType enum; type is compile-time, not runtime

## Impact

- **Sound engine library** (`SoundEngine/src/`): Add `name[32]` to Config struct, add VehicleType enum
- **ConfigParser** (`common/ConfigParser.h`): Absorb vehicle config parsing and sound loading from VehicleProfile
- **Config.h** (`common/Config.h`): Delete VehicleConfig struct
- **VehicleProfile** (`SoundEngine/src/VehicleProfile.h`): Simplify or delete
- **main.cpp** (RC_Truck, RC_Loco): Update boot sequence to use consolidated ConfigParser
- **JSON configs**: Remove TYPE field from vehicle JSONs (now a build flag)
- **platformio.ini**: Add `-D VEHICLE_TYPE_TRUCK` and `-D VEHICLE_TYPE_LOCOMOTIVE` build flags
