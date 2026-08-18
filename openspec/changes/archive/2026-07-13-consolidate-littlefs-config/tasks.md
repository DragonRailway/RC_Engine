## 1. Sound Engine Library Changes

- [x] 1.1 Add `VehicleType` enum to `SoundEngine/src/SoundTypes.h` (TRUCK, EXCAVATOR, LOCOMOTIVE, CAR, TANK, UNKNOWN)
- [x] 1.2 Add `char name[32]` field to `RcEngineSound::Config` struct
- [x] 1.3 Add `-D VEHICLE_TYPE_TRUCK=0` and `-D VEHICLE_TYPE_LOCOMOTIVE=1` build flags to platformio.ini

## 2. ConfigParser Consolidation

- [x] 2.1 Move `VehicleProfile::parseConfig()` logic into `ConfigParser::loadVehicleConfig()` — parse engine, transmission, sound volumes, features, loop points, mix weights into RcEngineSound::Config
- [x] 2.2 Add `ConfigParser::loadSounds(vehicleName, soundData)` — absorb sound loading logic from VehicleProfile::load()
- [x] 2.3 Add `ConfigParser::begin()` call to init LittleFS (absorb from SoundLoader::begin())

## 3. Cleanup

- [x] 3.1 Delete `VehicleConfig` struct from `Config.h`
- [x] 3.2 Simplify or delete `VehicleProfile` in `SoundEngine/src/VehicleProfile.h`
- [x] 3.3 Remove `SoundLoader` class from `SoundEngine/src/SoundLoader.h` (absorbed into ConfigParser)
- [x] 3.4 Remove `"TYPE"` field from all vehicle JSON configs in `RC_Truck/configs/`

## 4. App Integration

- [x] 4.1 Update `RC_Truck/src/main.cpp` boot sequence: ConfigParser::begin() → loadHardwareConfig() → loadVehicleConfig() → loadSounds() → HardwareInit::init()
- [x] 4.2 Update `RC_Loco/src/main.cpp` with same boot sequence pattern
- [x] 4.3 Verify printConfig() reads from RcEngineSound::Config instead of VehicleConfig

## 5. Verification

- [x] 5.1 Build RC_Truck environment (`pio run -e RC_Truck`) — must compile cleanly
- [x] 5.2 Build RC_Loco environment (`pio run -e RC_Loco`) — must compile cleanly
- [x] 5.3 Verify no references to deleted VehicleConfig struct remain
