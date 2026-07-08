# PSRAM Sound Loading Plan - IMPLEMENTED

## Status: ✅ Complete

## Current State
- `SoundData` struct already has `isDynamic` flag + destructor for heap/PSRAM
- JSON sound files exist in `src/sounds/` (22,050 Hz, 8-bit PCM, ~748KB total)
- `ConfigLoader` already handles LittleFS + ArduinoJson
- I2S driver NOT implemented yet (pins defined in `TRACKLINK_V3.h`)

## Implementation Plan

### Phase 1: SoundLoader (JSON → PSRAM)

**File: `src/SoundLoader.h`**

```
SoundLoader
├── begin()                    // Initialize LittleFS
├── loadSound(path) → SoundData*  // Load single JSON → PSRAM
├── unloadSound(SoundData*)    // Free PSRAM memory
└── loadVehicleProfile(profilePath) → VehicleProfile*
    // Load all sounds for a vehicle
```

**JSON Schema** (existing format):
```json
{
  "sampleRate": 22050,
  "sampleCount": 4406,
  "samples": [0, 4, 14, ...]
}
```

**PSRAM Allocation Strategy**:
- Allocate `int8_t*` array in PSRAM using `ps_malloc()` (ESP32 Arduino)
- Copy samples from JSON array → PSRAM buffer
- Set `isDynamic = true` so destructor frees correctly
- Track all allocations for cleanup

### Phase 2: VehicleProfile Manager

**File: `src/VehicleProfile.h`**

Maps vehicle config JSON → `SoundData` + `Config` structs:

```json
{
  "name": "ScaniaV8",
  "sounds": {
    "idle": "/sounds/idle-ScaniaV8.json",
    "rev": "/sounds/rev-ScaniaV8.json",
    "start": "/sounds/start-ScaniaV8.json",
    "knock": "/sounds/knock-ScaniaV8.json",
    "turbo": "/sounds/whistle-Turbo.json",
    "wastegate": "/sounds/wastegate-1000HpScaniaV8.json",
    "horn": "/sounds/horn-ScaniaV8train.json",
    "airbrake": "/sounds/airbrake-Truck2.json",
    "shifting": "/sounds/ClunkingGearShifting.json",
    "parkingbrake": "/sounds/parkingbrake-Generic.json",
    "reversing": "/sounds/reversing-TruckBeep.json",
    "siren": "/sounds/siren-Dummy.json",
    "fan": "/sounds/fan-Generic.json",
    "supercharger": "/sounds/supercharger.json",
    "coupling": "/sounds/coupling-generic.json",
    "uncoupling": "/sounds/uncoupling-generic.json",
    "indicator": "/sounds/indicator-Generic.json"
  },
  "config": {
    "acc": 2,
    "dec": 1,
    "inertia": 10,
    "maxRpmPercentage": 310,
    "automatic": false,
    ...
  }
}
```

### Phase 3: I2S Audio Output

**File: `src/AudioOutput.h`**

- Timer ISR at 22,050 Hz (45μs budget per sample)
- Call `RcEngineSound::getNextSample()` in ISR
- Write to I2S driver (already pinned in `TRACKLINK_V3.h`)
- Double-buffering for smooth playback

### Phase 4: Integration

1. `main.cpp` calls `VehicleProfile::load("vehicle-ScaniaV8.json")`
2. Profile manager loads all 17 JSON files → PSRAM
3. `RcEngineSound::begin(soundData, config)` with loaded data
4. I2S timer starts, ISR feeds samples
5. On profile switch: `unload()` frees PSRAM, `load()` new profile

## RAM Budget (ESP32-S3 N4R2: 2MB PSRAM)

| Resource | Usage | Notes |
|----------|-------|-------|
| PSRAM (2MB) | ~71KB per profile | 17 sounds × ~4KB avg |
| Internal RAM | ~35KB | JSON parsing buffer + stack |
| Free PSRAM | ~1.9MB | Plenty for other uses |

## File Changes

### New Files
- `src/SoundLoader.h` - JSON → PSRAM loader
- `src/VehicleProfile.h` - Profile manager
- `src/AudioOutput.h` - I2S driver + ISR
- `src/vehicle-profiles/*.json` - Vehicle config files (convert from headers)

### Modified Files
- `src/main.cpp` - Use new loader instead of static includes
- `lib/RcEngineSound/src/RcEngineSound.h` - Add fields for new sound layers (jakeBrake, fan, siren, shifting, brake, reversing, parkingBrake, supercharger)

### Reference Files
- `src/ConfigLoader.h` - Reuse LittleFS + JSON parsing patterns
- `src/sounds/*.json` - Existing sound data (18 files)
- `src/example_config/vehicle-config.json` - Config schema reference

## Testing Strategy

1. **Unit test**: Load single JSON → verify sample count + data integrity
2. **Integration test**: Load full vehicle profile → verify all 17 sounds loaded
3. **PSRAM test**: Verify `ps_malloc()` allocations, check for fragmentation
4. **Audio test**: I2S output → hear sound through speaker
5. **Profile switch test**: Load/unload multiple profiles → verify no memory leaks

## Decisions (Resolved)

1. **Vehicle profile JSON format**: Use existing `vehicle-config.json` schema
2. **Sound file naming**: Rename to `{Vehicle}-{type}.json` format (e.g., `ScaniaV8-idle.json`)
3. **Legacy header support**: No backward compatibility - remove `VehicleLibrary.h`
