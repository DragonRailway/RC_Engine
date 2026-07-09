# Design: Remaining Sound Gaps

## Architecture

All 5 new voice slots follow the same pattern as existing voices:
- Voice state in `VoiceState voices[VOICE_COUNT]`
- Sound data in `SoundData` struct
- Config volume in `Config` struct
- Trigger via public `triggerXxx(bool active)` methods
- Volume scaling in `update()` based on RPM/speed/state

## New Voice Slots

```
VOICE_TIRE_SQUEAL      — pitchShifted=false, loop=true
VOICE_HYDRAULIC_PUMP   — pitchShifted=true,  loop=true
VOICE_HYDRAULIC_FLOW   — pitchShifted=false, loop=true
VOICE_TRACK_RATTLE     — pitchShifted=false, loop=false (one-shot, speed-dependent interval)
VOICE_BUCKET_RATTLE    — pitchShifted=false, loop=false (one-shot)
```

## Trigger Logic

### Tire Squeal
```cpp
// In update(): activate when throttle high AND virtualSpeed low
bool squealCondition = (throttle > cfg.tireSquealThreshold && 
                        virtualSpeed < cfg.tireSquealMaxSpeed);
voices[VOICE_TIRE_SQUEAL].active = squealCondition;
// Volume: inversely proportional to speed
voices[VOICE_TIRE_SQUEAL].volume = cfg.tireSquealVolume * 
    (100 - virtualSpeed * 100 / cfg.maxRpm) / 100;
```

### Hydraulic Pump
```cpp
// In update(): activate when hydraulic mode enabled + engine running
voices[VOICE_HYDRAULIC_PUMP].active = cfg.hydraulicEnabled && state == RUNNING;
// Volume: RPM-dependent scaling
int32_t pumpScale = map(currentRpmFixed, 0, cfg.maxRpm, 30, 100);
voices[VOICE_HYDRAULIC_PUMP].volume = cfg.hydraulicPumpVolume * pumpScale / 100;
// Hydrostatic mode: additionally scale with speed
if (cfg.hydrostaticMode) {
    int32_t speedScale = map(virtualSpeed, 0, cfg.maxRpm, 50, 100);
    voices[VOICE_HYDRAULIC_PUMP].volume = voices[VOICE_HYDRAULIC_PUMP].volume * speedScale / 100;
}
```

### Hydraulic Flow
```cpp
// External trigger: triggerHydraulicFlow(bool active)
// Called from main.cpp when boom/bucket movement detected
void triggerHydraulicFlow(bool active) {
    voices[VOICE_HYDRAULIC_FLOW].active = active;
    if (active) voices[VOICE_HYDRAULIC_FLOW].position = 0;
}
```

### Track Rattle
```cpp
// In update(): interval-based one-shot triggering
if (cfg.trackRattleEnabled && virtualSpeed > 0) {
    uint32_t interval = map(virtualSpeed, 0, cfg.maxRpm,
                           cfg.trackRattleIntervalMax, cfg.trackRattleIntervalMin);
    if (now - lastTrackRattleTime > interval) {
        voices[VOICE_TRACK_RATTLE].active = true;
        voices[VOICE_TRACK_RATTLE].position = 0;
        lastTrackRattleTime = now;
    }
}
```

### Bucket Rattle
```cpp
// External trigger: triggerBucketRattle(bool active)
// One-shot per movement event
void triggerBucketRattle(bool active) {
    voices[VOICE_BUCKET_RATTLE].active = active;
    if (active) voices[VOICE_BUCKET_RATTLE].position = 0;
}
```

## Dump Bed Mode
```cpp
// Config flag: cfg.dumpBedEnabled
// When dump bed button pressed, activates hydraulic pump + flow
void triggerDumpBed(bool active) {
    if (cfg.dumpBedEnabled) {
        voices[VOICE_HYDRAULIC_PUMP].active = active;
        voices[VOICE_HYDRAULIC_FLOW].active = active;
        if (active) {
            voices[VOICE_HYDRAULIC_PUMP].position = 0;
            voices[VOICE_HYDRAULIC_FLOW].position = 0;
        }
    }
}
```

## Config Fields (JSON)

```json
{
  "SOUND_VOLUME": {
    "TIRE_SQUEAL": 160,
    "HYDRAULIC_PUMP": 120,
    "HYDRAULIC_FLOW": 20,
    "TRACK_RATTLE": 150,
    "BUCKET_RATTLE": 160
  },
  "FEATURES": {
    "TIRE_SQUEAL_THRESHOLD": 70,
    "TIRE_SQUEAL_MAX_SPEED": 30,
    "HYDRAULIC_ENABLED": false,
    "HYDROSTATIC_MODE": false,
    "TRACK_RATTLE_ENABLED": false,
    "TRACK_RATTLE_INTERVAL_MIN": 90,
    "TRACK_RATTLE_INTERVAL_MAX": 500,
    "DUMP_BED_ENABLED": false
  }
}
```

## Files Modified

| File | Changes |
|------|---------|
| `RcEngineSound.h` | +5 voice slots, +Config fields, +trigger methods, +state vars |
| `RcEngineSound.cpp` | +trigger methods, +update() logic, +begin() voice setup, +getNextSample() |
| `VehicleProfile.h` | +parseConfig() for new fields, +populateSoundData() |
| `vehicle-config.json` | +new volume/feature fields |
| `vehicle-ScaniaV8.json` | +new volume/feature fields |

## Memory Impact

- +5 voice slots × ~32 bytes = ~160 bytes RAM
- +5 SoundData pointers = ~20 bytes RAM
- +Config fields = ~24 bytes RAM
- **Total: ~204 bytes additional RAM** (negligible on ESP32-S3 with 512KB)
