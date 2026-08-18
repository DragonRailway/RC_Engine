# Tasks: Remaining Sound Gaps

## Task 1: Add 5 new voice slots to VoiceID enum and VoiceState array
- File: `SoundEngine/src/RcEngineSound.h`
- Add `VOICE_TIRE_SQUEAL`, `VOICE_HYDRAULIC_PUMP`, `VOICE_HYDRAULIC_FLOW`, `VOICE_TRACK_RATTLE`, `VOICE_BUCKET_RATTLE` to `VoiceID` enum
- Update `VOICE_COUNT` from 19 to 24
- Add corresponding `SoundData` fields: `tireSquealSamples`, `hydraulicPumpSamples`, `hydraulicFlowSamples`, `trackRattleSamples`, `bucketRattleSamples` + counts

## Task 2: Add Config fields for new features
- File: `SoundEngine/src/RcEngineSound.h`
- Add volume fields: `tireSquealVolume`, `hydraulicPumpVolume`, `hydraulicFlowVolume`, `trackRattleVolume`, `bucketRattleVolume`
- Add feature flags: `hydraulicEnabled`, `hydrostaticMode`, `trackRattleEnabled`, `dumpBedEnabled`
- Add tire squeal params: `tireSquealThreshold`, `tireSquealMaxSpeed`
- Add track rattle params: `trackRattleIntervalMin`, `trackRattleIntervalMax`

## Task 3: Add trigger methods
- File: `SoundEngine/src/RcEngineSound.h` + `.cpp`
- Add `triggerTireSqueal(bool active)`
- Add `triggerHydraulicPump(bool active)`
- Add `triggerHydraulicFlow(bool active)`
- Add `triggerTrackRattle(bool active)`
- Add `triggerBucketRattle(bool active)`
- Add `triggerDumpBed(bool active)` — activates pump + flow together

## Task 4: Add state variables
- File: `SoundEngine/src/RcEngineSound.h`
- Add `uint32_t lastTrackRattleTime = 0;` for interval-based triggering
- Add `uint32_t lastTrackRattle2Time = 0;` for dual rattle (optional)

## Task 5: Update begin() to configure new voices
- File: `SoundEngine/src/RcEngineSound.cpp`
- Wire up samples, counts, pitchShifted, loop, oneShot for each new voice
- Set default volumes from config

## Task 6: Update destructor to free dynamic memory
- File: `SoundEngine/src/RcEngineSound.cpp`
- Add `free()` calls for new sound sample pointers in destructor

## Task 7: Add tire squeal trigger logic to update()
- File: `SoundEngine/src/RcEngineSound.cpp`
- Detect: `throttle > cfg.tireSquealThreshold && virtualSpeed < cfg.tireSquealMaxSpeed`
- Volume: inversely proportional to speed
- Loop while conditions met

## Task 8: Add hydraulic pump logic to update()
- File: `SoundEngine/src/RcEngineSound.cpp`
- Activate when `cfg.hydraulicEnabled && state == RUNNING`
- Volume: RPM-dependent scaling (30-100%)
- Hydrostatic mode: additionally scale with vehicle speed

## Task 9: Add track rattle interval logic to update()
- File: `SoundEngine/src/RcEngineSound.cpp`
- Interval-based one-shot: `map(virtualSpeed, 0, maxRpm, intervalMin, intervalMax)`
- Trigger voice when interval elapsed AND speed > 0

## Task 10: Update VehicleProfile to parse new config fields
- File: `SoundEngine/src/VehicleProfile.h`
- Add sound type names and generic names for new sounds
- Update `SOUND_COUNT` from 19 to 24
- Update `parseConfig()` to read new volume and feature fields
- Update `populateSoundData()` to map new sound indices

## Task 11: Update vehicle config JSONs
- Files: `RC_Truck/configs/vehicle-config.json`, `vehicle-ScaniaV8.json`
- Add new volume fields with default 0 (disabled)
- Add feature flags section

## Task 12: Build and verify compilation
- Run `pio run` and fix any errors

## Task 13: Code review
- Spawn code-reviewer-mimo to review all changes
