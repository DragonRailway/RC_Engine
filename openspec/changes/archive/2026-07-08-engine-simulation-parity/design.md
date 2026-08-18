# Engine Simulation Parity — Design

## Architecture Overview

```
┌──────────────────────────────────────────────────────────────────┐
│                  CURRENT ARCHITECTURE                            │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│  Timer ISR → getNextSample() → I2S DMA buffer → MAX98357A     │
│                                                                  │
│  • Fixed timer rate (22,050 Hz)                                 │
│  • No pitch shifting — all samples play at native rate         │
│  • Single state machine: OFF→START→RUN→STOP                    │
│  • 8-bit samples mixed to 16-bit I2S output                   │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────┐
│                  TARGET ARCHITECTURE                             │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│  I2S DMA callback → fillBuffer() → I2S peripheral → MAX98357A │
│                                                                  │
│  • Fixed I2S clock (22,050 Hz)                                  │
│  • Fractional step interpolation for pitch shifting             │
│  • Extended state machine: OFF→START→RUN→STOP→PARKING→OFF     │
│  • Per-voice pitch tracking with linear interpolation           │
│  • Cylinder-adaptive knock volume patterns                     │
│  • Auto-transmission with torque converter simulation          │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

## Feature 1: Variable-Rate Sample Playback (Pitch Shifting)

### Approach: Fractional Step Interpolation

All engine voices share a single pitch factor derived from RPM. Effect voices (horn, siren, brake, etc.) play at native rate (step = 1.0).

**Pitch calculation:**
```
pitchFactor = 1.0 + (currentRpm / maxRpm) * (maxPitchFactor - 1.0)
// At idle (RPM=0):  pitchFactor = 1.0  (native pitch)
// At full (RPM=500): pitchFactor = 3.3  (3.3x faster = higher pitch)
```

**Per-voice state:**
```cpp
struct VoiceState {
    float position;      // Fractional position in sample array
    float step;          // Step size (= pitchFactor for engine voices, 1.0 for effects)
    int8_t* samples;     // Pointer to sample data
    uint32_t count;      // Total sample count
    uint8_t volume;      // Per-voice volume (0-100)
    bool active;         // Is this voice playing?
    bool pitchShifted;   // Does this voice use pitch factor?
};
```

**Interpolation (per sample per voice):**
```
readPos = floor(position)
frac = position - readPos
sample = samples[readPos] * (1.0 - frac) + samples[readPos + 1] * frac
position += step
if (position >= count) position -= count  // Loop
```

**CPU budget:** ~32 cycles per voice × 8 voices = 256 cycles/sample. Budget is 10,884 cycles. Usage: 2.6%.

**Engine voices (pitch-shifted):** idle, rev, turbo, knock, fan, supercharger, jake brake
**Effect voices (fixed pitch):** horn, siren, brake, shifting, reversing, indicator, coupling, wastegate, parking brake, sound1, uncoupling

### State-specific behavior

| State | Engine voice step | Effect voices |
|-------|------------------|---------------|
| STARTING | 1.0 (fixed) | Disabled |
| RUNNING | pitchFactor (varies with RPM) | Active as triggered |
| STOPPING | Ramps from current pitchFactor → 1.0 | Disabled |
| PARKING_BRAKE | 0 (silent) | Parking brake one-shot |

## Feature 2: Diesel Knock Cylinder-Adaptive Volume

### Approach: Trigger-Based

Instead of looping the knock sample continuously, trigger individual knock pulses at intervals derived from the idle loop position.

**Timing:**
```
knockInterval = idleSampleCount / dieselKnockInterval
// Every knockInterval samples of idle playback → trigger one knock pulse
```

**Cylinder patterns:**
```
V8:    [1,2,3,4,5,6,7,8] → loud at positions 4, 8
V8_468:[1..16]           → loud at positions 1, 5, 9, 13
R6:    [1,2,3,4,5,6]     → loud at position 6
R6_2:  [1,2,3,4,5,6]     → loud at positions 3, 6
V2:    [1,2,3,4]         → loud at positions 1, 2
UNIFORM: all equal volume
```

**Volume:**
- Primary pulses (loud): 100% of configured knock volume
- Secondary pulses: knockAdaptiveVolume % of primary (e.g., 18%)

**Knock sample:** Single pulse (not a loop). Triggered N times per idle cycle.

## Feature 3: Jake Brake Engine Slowdown

### Approach: Throttle-Based Auto-Trigger

**Activation conditions:**
1. Engine state = RUNNING
2. currentRpm > jakeBrakeMinRpm
3. Throttle released (targetRpm < currentRpm)

**When active:**
- RPM decelerates at jakeBrakeDecelRate (faster than normal)
- Idle/rev/turbo/knock/fan/supercharger sounds are MUTED
- Jake brake sound plays (pitch-shifted with engine)
- Jake brake volume scales with RPM

**When deactivated:**
- RPM falls below jakeBrakeMinRpm
- Normal engine sounds resume

## Feature 4: PARKING_BRAKE State

### Shutdown Sequence

```
RUNNING → STOPPING → PARKING_BRAKE → OFF
```

**STOPPING phase:**
- Volume fades (attenuator++)
- Pitch drops (pitchFactor ramps from current → 1.0)
- Duration: ~3.2 seconds (40 steps × 80ms)

**PARKING_BRAKE phase:**
- Engine sound completely silent
- Parking brake one-shot sound plays
- Waits for sound to finish
- Transitions to OFF

## Feature 5: Automatic Transmission Simulation

### Torque Converter Model

**Gear mapping:**
```
gearSize = maxRpm / numberOfGears
currentGear = throttle / gearSize
gearThrottle = throttle % gearSize
targetRpm = currentGear * gearSize + gearThrottle
```

**Per-gear ramp times:**
- Gear 1: fast response (configurable)
- Gear N: slower response (heavier feel)

**When TYPE = "MANUAL" or "NONE":**
- No gear simulation
- RPM follows throttle directly (current behavior)

## Feature 6: Supercharger Start Point

Below `superchargerStartPoint` RPM%: volume = 0
Above: volume scales from 0 → 100% based on RPM

## Feature 7: Uncoupling Separate Sound

Add `uncouplingSamples` to SoundData and `triggerUncoupling()` method. VehicleProfile loads both `coupling.json` and `uncoupling.json`.

## Feature 8: Sound1 Generic Channel

Extra sound slot triggered on demand. Plays once when triggered, loops if held. Volume configurable.

## Feature 9: ESC Ramp Time Per Gear

Array of ramp times indexed by gear number. Used by automatic transmission to vary acceleration response per gear.
