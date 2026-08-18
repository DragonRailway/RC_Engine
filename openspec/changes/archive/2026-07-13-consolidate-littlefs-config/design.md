## Context

The RC Brain project has two ESP32 apps (RC_Truck, RC_Loco) sharing a common config system. Currently, vehicle JSON configs are parsed twice by independent code paths — `ConfigParser` fills an incomplete `VehicleConfig` struct, and `VehicleProfile::parseConfig()` fills the complete `RcEngineSound::Config`. The vehicle type (`TRUCK`, `EXCAVATOR`, etc.) is stored in JSON but is actually a compile-time constant per app. This creates unnecessary complexity and maintenance burden.

**Current state:**
- `Config.h`: `VehicleConfig` struct (~30 fields parsed)
- `ConfigParser.h`: `loadHardwareConfig()` + `loadVehicleConfig()` (incomplete)
- `VehicleProfile.h`: `parseConfig()` (~70 fields parsed) + sound loading
- JSON files: contain `TYPE` field that's redundant with build flags

**Constraints:**
- Header-only codebase (all `.h` files, no `.cpp` in common/)
- ArduinoJson ^7.0.4 with comments enabled
- LittleFS on ESP32-S3 (4MB flash)
- Sound engine library is shared across apps

## Goals / Non-Goals

**Goals:**
- Single parse path: ConfigParser handles all JSON parsing
- Single config struct: `RcEngineSound::Config` is the vehicle config (with `name[32]` added)
- Vehicle type as compile-time enum, not runtime string
- Delete `VehicleConfig` struct (BREAKING)
- Simplify or delete `VehicleProfile`

**Non-Goals:**
- Runtime vehicle selection (TBD later)
- Web UI or serial menu for config editing
- Config validation beyond JSON deserialization
- Changing hardware config structure

## Decisions

### Decision 1: ConfigParser absorbs VehicleProfile's parsing

**Choice:** Move `VehicleProfile::parseConfig()` logic into `ConfigParser::loadVehicleConfig()`, and move sound loading into `ConfigParser::loadSounds()`.

**Rationale:** Single parser = single authority. ConfigParser already handles hardware JSON, adding vehicle JSON is natural. VehicleProfile becomes a thin container or gets deleted.

**Alternative considered:** Keep VehicleProfile as parser, delete ConfigParser. Rejected because ConfigParser already has the JSON/file infrastructure and hardware parsing.

### Decision 2: VehicleType enum in library

**Choice:** Add `enum VehicleType { TRUCK, EXCAVATOR, LOCOMOTIVE, ... }` to the sound engine library. Each app sets its type via build flag (`-D VEHICLE_TYPE_TRUCK`). The JSON `TYPE` field is removed.

**Rationale:** Vehicle type determines engine behavior (knock patterns, transmission logic, sound mixing). It's a compile-time constant per app, not a user-configurable setting.

**Alternative considered:** Keep type in JSON. Rejected because it's redundant with build flags and could be accidentally changed.

### Decision 3: Add name to RcEngineSound::Config

**Choice:** Add `char name[32]` to the Config struct. JSON field: `"VEHICLE": { "NAME": "Scania V8" }`.

**Rationale:** Vehicle name is the only runtime variable that differs per profile. Type is compile-time, everything else is tuning parameters.

### Decision 4: ConfigParser returns SoundData directly

**Choice:** `ConfigParser::loadSounds(vehicleName)` returns `SoundData` (or fills a reference). Absorbs `SoundLoader::loadSound()` and `VehicleProfile::load()` sound loading logic.

**Rationale:** Sound files are part of the vehicle config. One call loads everything.

## Risks / Trade-offs

- **[Risk]** Engine library modification required → Adding `name` to Config means editing external lib. **Mitigation:** Minimal change (one field), well-scoped.

- **[Risk]** Breaking change for existing configs → Removing `TYPE` from JSON. **Mitigation:** Update example configs, document in migration notes.

- **[Trade-off]** Less separation between app and engine → App reads `RcEngineSound::Config` directly. **Mitigation:** Acceptable because the engine library is project-owned, not a third-party dependency.

- **[Trade-off]** ConfigParser grows larger → Absorbs vehicle parsing + sound loading. **Mitigation:** Each method is still focused (loadHardware, loadVehicle, loadSounds).
