## 1. Configuration & Fallback

- [x] 1.1 Add `"bell": 120` to `configs/vehicle_configs/UnionPacific2002/vehicle.json`
- [x] 1.2 Add locomotive fallback `(cfg.type == RcEngineSound::VEHICLE_LOCOMOTIVE ? 100 : 0)` to `ConfigParser::parseSoundVolumes()` in `common/ConfigParser.cpp`

## 2. Sound Engine Volume Synchronization

- [x] 2.1 Update `RcEngineSound::setConfig()` to synchronize voice volumes on config updates in `lib/SoundEngine/src/RcEngineSound.h` and `RcEngineSound.cpp`

## 3. Verification & Flash

- [x] 3.1 Run host tests (`host_dsp_test.py`, `host_vc_test.py`, `validate_configs.py`)
- [x] 3.2 Flash updated LittleFS image to `TRACKLINK_V3` (`python3 scripts/build_fs.py --board TRACKLINK_V3 --vehicle UnionPacific2002`)
- [x] 3.3 Verify bell sound playback on hardware
