## Context

On locomotive profiles, `bell_button` sends push-button events to `VehicleController`, which invokes `RcEngineSound::triggerBell()`. However, because `UnionPacific2002/vehicle.json` omitted `"bell"` from `sound_volumes`, the volume was 0, resulting in silent playback.

## Goals / Non-Goals

**Goals:**
- Configure `"bell": 120` in `UnionPacific2002/vehicle.json`.
- Add an automatic locomotive default fallback `(cfg.type == RcEngineSound::VEHICLE_LOCOMOTIVE ? 100 : 0)` in `ConfigParser::parseSoundVolumes()`.
- Update `RcEngineSound::setConfig()` and `applyVoiceVolumes()` so volume changes apply on config reload.
- Flash and verify bell playback on `TRACKLINK_V3` hardware.

**Non-Goals:**
- Modifying RadioKit UI widget geometry or protocol.

## Decisions

### 1. `UnionPacific2002/vehicle.json`
```json
"sound_volumes": {
  "start": 160,
  "idle": 90,
  "bell": 120,
  "engine_idle": 60,
  ...
}
```

### 2. Smart Fallback in `ConfigParser.cpp`
```cpp
cfg.sound.bell = sv["bell"] | sv["BELL"] | (cfg.type == RcEngineSound::VEHICLE_LOCOMOTIVE ? 100 : 0);
```

### 3. Voice Volume Sync in `RcEngineSound.cpp`
Extract a helper `applyVoiceVolumes()` in `RcEngineSound.cpp` that assigns `voices[...].volume = cfg.sound....` and call it from both `begin()` and `setConfig()`.
