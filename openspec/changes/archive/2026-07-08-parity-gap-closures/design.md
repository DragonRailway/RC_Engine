# Parity Gap Closures — Design

## Architecture Overview

All changes extend existing structures in `RcEngineSound.h/cpp`. No new files needed. Each feature is self-contained and independently testable.

```
┌─────────────────────────────────────────────────────────────────┐
│                    DESIGN OVERVIEW                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  VoiceState (existing)                                          │
│  ├── position, step, samples, count, volume, active             │
│  ├── pitchShifted, oneShot, loop                                │
│  └── NEW: loopBegin, loopEnd  ← Feature 1 (loop points)       │
│                                                                  │
│  Config (existing)                                              │
│  ├── All engine parameters                                      │
│  └── NEW: minKnockVolume, knockStartRpm  ← Feature 3           │
│  └── NEW: crawlerModeThreshold  ← Feature 6                    │
│                                                                  │
│  RcEngineSound (existing)                                       │
│  ├── update(), getNextSample()                                  │
│  └── NEW: crawlerMode bool  ← Feature 6                        │
│                                                                  │
│  AudioOutput (existing)                                         │
│  └── NEW: offsetFade logic  ← Feature 2                        │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

## Feature 1: Horn/Siren/Reversing Loop Points

### Current Behavior
When `triggerHorn(true)` is called, the horn voice plays from position 0 to count-1, then deactivates. If the trigger is still held, the horn stops.

### New Behavior
When the voice reaches `loopEnd`, it jumps back to `loopBegin` and continues playing as long as the trigger is active.

### Implementation

```
VoiceState additions:
  uint32_t loopBegin = 0;    // Start of loop region (default: 0)
  uint32_t loopEnd = 0;      // End of loop region (default: 0 = use full count)

advanceVoice() modification:
  if (v.loop && v.loopEnd > 0) {
      // Loop within defined region
      if (v.position >= v.loopEnd) {
          v.position = v.loopBegin;
      }
  } else if (v.loop) {
      // Original behavior: loop full sample
      if (v.position >= v.count) v.position -= v.count;
  } else {
      // One-shot: deactivate at end
      if (v.position >= v.count) {
          v.position = 0;
          v.active = false;
      }
  }
```

### Config additions
```json
{
  "LOOP_POINTS": {
    "HORN_BEGIN": 0,
    "HORN_END": 0,
    "SIREN_BEGIN": 0,
    "SIREN_END": 0,
    "REVERSING_BEGIN": 0,
    "REVERSING_END": 0,
    "SOUND1_BEGIN": 0,
    "SOUND1_END": 0
  }
}
```

Values of 0 mean "use full sample length" (backward compatible).

## Feature 2: DAC Offset Fade (Pop Prevention)

### Current Behavior
I2S output starts with offset=128 immediately, causing a pop on power-up.

### New Behavior
Offset ramps from 0→128 over ~12ms on first `writeSample()` call.

### Implementation

In `AudioOutput.h`:
```cpp
bool offsetInitialized = false;
uint8_t currentOffset = 0;

void writeSample(int8_t sample) {
    if (!offsetInitialized) {
        // Ramp offset over first ~276 samples (12ms at 22050Hz)
        currentOffset = min((uint8_t)(currentOffset + 1), (uint8_t)128);
        if (currentOffset == 128) offsetInitialized = true;
    }
    uint8_t value = constrain((int16_t)sample + currentOffset, 0, 255);
    // write to I2S...
}
```

## Feature 3: RPM-Dependent Knock Volume

### Current Behavior
Knock volume is fixed at `knockVolume * knockAdaptiveVolume / 100` for secondary pulses, and `knockVolume` for primary pulses. No RPM scaling.

### New Behavior
Knock volume scales with RPM: at idle, knock is quiet; at high RPM, knock is full volume. Uses a linear map from `knockStartRpm` to `maxRpm`.

### Implementation

In `update()`, when computing knock volume:
```cpp
// RPM-dependent knock scaling
uint16_t knockRpmThreshold = (uint16_t)(cfg.maxRpm * cfg.knockStartRpm / 100);
uint16_t minKnockVol = (uint16_t)(cfg.knockVolume * cfg.minKnockVolume / 100);

if (currentRpmFixed > knockRpmThreshold) {
    uint16_t rpmScale = map(currentRpmFixed, knockRpmThreshold, cfg.maxRpm, minKnockVol, cfg.knockVolume);
    voices[VOICE_KNOCK].volume = isLoud ? rpmScale : (uint16_t)(rpmScale * cfg.knockAdaptiveVolume / 100);
} else {
    voices[VOICE_KNOCK].volume = isLoud ? minKnockVol : (uint16_t)(minKnockVol * cfg.knockAdaptiveVolume / 100);
}
```

### Config additions
```json
{
  "KNOCK_START_RPM": 10,
  "MIN_KNOCK_VOLUME": 80
}
```

## Feature 4: Manual Transmission Shifting Trigger

### Current Behavior
The shifting sound slot exists but is only triggered by external `triggerShifting()` call.

### New Behavior
When `transmissionType == MANUAL` and `selectedGear` changes in `update()`, automatically trigger the shifting sound.

### Implementation

In `update()`:
```cpp
static uint8_t lastGear = 1;
if (cfg.transmissionType == TRANS_MANUAL && state == RUNNING) {
    // Gear selection based on RPM ranges
    int32_t gearSize = cfg.maxRpm / cfg.numberOfGears;
    if (gearSize > 0) {
        uint8_t newGear = (uint8_t)(targetRpm / gearSize) + 1;
        if (newGear > cfg.numberOfGears) newGear = cfg.numberOfGears;
        if (newGear != lastGear) {
            voices[VOICE_SHIFTING].active = true;
            voices[VOICE_SHIFTING].position = 0;
            lastGear = newGear;
        }
        selectedGear = newGear;
    }
}
```

## Feature 5: Voice Mixing Weights

### Current Behavior
All voices are mixed with equal weight in `getNextSample()`. When many effects play simultaneously, clipping can occur.

### New Behavior
Engine voices and effect voices have separate volume groups, mixed with configurable weights before final sum.

### Implementation

In `getNextSample()`:
```cpp
int32_t engineMix = 0;
int32_t effectMix = 0;

for (int i = 0; i < VOICE_COUNT; i++) {
    VoiceState& v = voices[i];
    if (!v.active || !v.samples || v.count == 0) continue;
    // ... read sample, apply volume ...
    if (v.pitchShifted) {
        engineMix += scaled;
    } else {
        effectMix += scaled;
    }
}

// Apply group weights (reference uses 8/10 engine, 2/10 effects on separate DACs)
// Since we have one channel, we balance differently:
mixed = engineMix + effectMix;
mixed = (mixed * cfg.masterVolume / 100) + 128;
```

### Config additions
```json
{
  "ENGINE_MIX_WEIGHT": 100,
  "EFFECT_MIX_WEIGHT": 100
}
```

## Feature 6: Crawler Mode

### Current Behavior
Virtual inertia is always active, making RPM response slow.

### New Behavior
When master volume is below `crawlerModeThreshold`, inertia is disabled (instant RPM response).

### Implementation

In `update()`:
```cpp
bool crawlerMode = (cfg.masterVolume <= cfg.crawlerModeThreshold);

if (crawlerMode) {
    currentRpm = effectiveTarget; // Instant response
} else {
    // Normal inertia calculation
    int32_t inertiaFactor = max((int32_t)1, (int32_t)(101 - cfg.inertia));
    // ... existing inertia logic ...
}
```

### Config additions
```json
{
  "CRAWLER_MODE_THRESHOLD": 44
}
```
