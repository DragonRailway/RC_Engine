## 1. Dynamic Config Parsing

- [x] 1.1 Add `VehicleType` enum (`TRUCK`, `LOCOMOTIVE`, `EXCAVATOR`, `CUSTOM`) to `RcEngineSound::Config` or `Config.h`
- [x] 1.2 Update `ConfigParser::loadVehicleConfig()` to parse `VEHICLE.TYPE` string into `VehicleType` enum (defaulting to `TRUCK`)
- [x] 1.3 Update `ConfigParser::loadSounds()` to support resolving sound files with vehicle-prefixed names (`/sounds/{NAME}-{type}.json`) with generic fallback

## 2. Unified Entrypoint & Build System

- [x] 2.1 Create unified `src/main.cpp` with LittleFS initialization, hardware config parsing, vehicle config parsing, sound engine startup, and boot error handling
- [x] 2.2 Update `platformio.ini` to consolidate environments into `[env:TRACKLINK_V3]` with `src_dir = src`, removing `-D RC_TRUCK` and `-D RC_LOCO` build flags
- [x] 2.3 Remove legacy split directories `RC_Loco/` and `RC_Truck/`

## 3. Build & Runtime Verification

- [x] 3.1 Run `pio run` to verify clean compilation of unified firmware
- [x] 3.2 Run test suite `pio test` to confirm test suite stability
