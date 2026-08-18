# Design: Architectural Improvements

## 1. Fix Double DC Offset (AudioOutput.h)

**Current flow:**
```
getNextSample() → returns uint8_t with +128 offset
onTimer() → multiplies by 256 → int16_t centered at 32768
audioTask() → adds currentOffset (0-128) → double offset
```

**Fix:** Remove +128 from `getNextSample()`. The offset ramp in `audioTask()` will handle centering from silence (0) to center (128).

```cpp
// getNextSample() — remove the +128
mixed = (mixed * cfg.masterVolume / 100);  // returns centered at 0
return (uint8_t)constrain(mixed + 128, 0, 255);  // still uint8_t for ISR
```

Wait — the ISR needs uint8_t. Better approach: keep getNextSample returning 0-255 with +128, but fix audioTask to NOT add offset. Instead, scale the entire buffer:

```cpp
// audioTask — scale instead of offset
float scale = currentOffset / 128.0f;  // 0.0 → 1.0
for (int i = 0; i < BUFFER_SIZE; i++) {
    buffer[i] = (int16_t)(buffer[i] * scale);
}
```

This fades from silence to full volume over ~12ms.

## 2. Fix ISR/Main Loop Thread Safety

**Approach:** Use `std::atomic` for the fields that are written by `update()` and read by `getNextSample()`.

Fields to make atomic:
- `VoiceState::active` (bool)
- `VoiceState::volume` (uint8_t)
- `VoiceState::position` (float — use atomic<uint32_t> with bit-cast)

Alternative (simpler): Copy voice state into an ISR-safe buffer at the start of `getNextSample()` under a brief critical section (portENTER_CRITICAL / portEXIT_CRITICAL). Since `getNextSample()` runs at 22,050 Hz (~45µs period), a critical section of ~1µs is acceptable.

**Chosen approach:** Critical section copy. It's simpler and avoids atomic complexity on float.

```cpp
uint8_t RcEngineSound::getNextSample() {
    // Snapshot voice state under critical section
    VoiceState snapshot[VOICE_COUNT];
    portENTER_CRITICAL(&voiceMutex);
    memcpy(snapshot, voices, sizeof(voices));
    portEXIT_CRITICAL(&voiceMutex);
    
    // Mix using snapshot (no race)
    for (int i = 0; i < VOICE_COUNT; i++) {
        // ... use snapshot[i] ...
    }
}
```

## 3. Refactor SoundData to Indexed Array

**Current:** 48 named fields (24 pairs of samples/count)

**New:** Single array indexed by SoundIndex

```cpp
struct SoundSlot {
    int8_t* samples = nullptr;
    uint32_t sampleCount = 0;
    uint16_t sampleRate = 22050;
};

struct SoundData {
    SoundSlot slots[SOUND_COUNT];
    bool isDynamic = false;
};
```

Access: `sounds.slots[IDLE].samples` instead of `sounds.samples`

**Impact:** Destructor becomes a loop. begin() becomes a loop. populateSoundData() becomes a loop.

## 4. Unify VoiceID and SoundIndex

**Current:** Two separate enums that must stay in sync

**New:** Single enum in a shared header `SoundTypes.h`

```cpp
enum SoundID {
    IDLE, REV, START, KNOCK, TURBO, WASTEGATE, HORN,
    JAKE_BRAKE, FAN, SIREN, BRAKE, PARKING_BRAKE,
    SHIFTING, REVERSING, INDICATOR, COUPLING, SUPERCHARGER,
    UNCOUPLING, SOUND1, TIRE_SQUEAL, HYDRAULIC_PUMP,
    HYDRAULIC_FLOW, TRACK_RATTLE, BUCKET_RATTLE,
    SOUND_COUNT
};
```

Both `RcEngineSound` and `VehicleProfile` use this single enum.

## 5. Group Config into Sub-structs

```cpp
struct Config {
    struct Engine {
        uint8_t acc = 2, dec = 2, inertia = 10;
        uint16_t maxRpm = 500;
        float maxPitchFactor = 3.3f;
        uint16_t revSwitchPoint = 50, idleEndPoint = 300;
        KnockPattern knockPattern = KNOCK_V8;
        uint8_t knockInterval = 8, knockAdaptiveVolume = 18;
        uint8_t minKnockVolume = 80, knockStartRpm = 10;
        uint8_t jakeBrakeMinRpm = 60, jakeBrakeDecelRate = 5;
        uint8_t superchargerStartPoint = 10;
    } engine;
    
    struct Sound {
        uint8_t master = 100;
        uint8_t volumes[SOUND_COUNT] = {};  // indexed by SoundID
        uint8_t engineMixWeight = 100, effectMixWeight = 100;
    } sound;
    
    struct Transmission {
        Type type = TRANS_NONE;
        uint8_t numberOfGears = 3;
        uint8_t gearRampTimes[6] = {20, 50, 75, 75, 75, 75};
    } transmission;
    
    struct Features {
        bool hydraulicEnabled = false;
        bool hydrostaticMode = false;
        bool trackRattleEnabled = false;
        bool dumpBedEnabled = false;
        uint8_t tireSquealThreshold = 70;
        uint8_t tireSquealMaxSpeed = 30;
        uint16_t trackRattleIntervalMin = 90;
        uint16_t trackRattleIntervalMax = 500;
    } features;
    
    struct LoopPoints {
        uint32_t hornBegin = 0, hornEnd = 0;
        uint32_t sirenBegin = 0, sirenEnd = 0;
        uint32_t reversingBegin = 0, reversingEnd = 0;
        uint32_t sound1Begin = 0, sound1End = 0;
    } loopPoints;
    
    uint8_t crawlerModeThreshold = 44;
    uint16_t wastegateThrottleDrop = 150;
    uint16_t wastegateMinRpm = 200;
};
```

## 6. Minor Cleanups

- Remove unused `hornSampleRate` from SoundSlot
- Remove `idx < 0` check in VehicleProfile::getSound (enum is unsigned)
- Remove legacy `automatic`, `clutchEngagingPoint`, `maxRpmPercentage` fields
- Move `SOUND_COUNT` before `sounds[]` array in VehicleProfile

## File Changes

| File | Changes |
|------|---------|
| NEW: `SoundTypes.h` | Shared SoundID enum, SOUND_COUNT constant |
| `RcEngineSound.h` | Use SoundID, Config sub-structs, VoiceState atomic/copy, mutex |
| `RcEngineSound.cpp` | Loop-based begin(), critical section in getNextSample(), fix offset |
| `VehicleProfile.h` | Use SoundID, indexed SoundData, remove duplicates |
| `SoundLoader.h` | Use SoundSlot instead of named fields |
| `AudioOutput.h` | Fix offset ramp to scale instead of add |

## Memory Impact

- SoundData: 48 fields → 24 slots × 12 bytes = 288 bytes (same)
- Config: ~60 fields → same total, better organized
- VoiceState snapshot: 24 × ~28 bytes = 672 bytes stack (temporary in ISR)
- Mutex: 1 portMUX_TYPE = 4 bytes

**Net change:** +676 bytes stack usage in getNextSample (acceptable for 4KB audio task stack)
