# Engine Simulation Parity — Tasks

## Implementation Order

Features are ordered by dependency — pitch shifting is the foundation that everything else builds on.

---

### Task 1: Pitch Shifting Infrastructure

**Depends on:** None
**Files:** `RcEngineSound.h`, `RcEngineSound.cpp`, `AudioOutput.h`

- [x] Add `VoiceState` struct to `RcEngineSound.h` with position, step, samples, count, volume, active, pitchShifted fields
- [x] Add voice state array `voices[19]` to `RcEngineSound` class (19 slots for all sound types)
- [x] Add `pitchFactor` float to Config (computed from RPM in update())
- [x] Add `maxPitchFactor` to Config (default 3.3, configurable per vehicle)
- [x] Modify `getNextSample()` to use fractional step interpolation for each voice
- [x] Add linear interpolation: `sample = samples[pos] * (1-frac) + samples[pos+1] * frac`
- [x] Add position advancement: `position += step; if (position >= count) position -= count`
- [x] Set engine voices (idle, rev, turbo, knock, fan, supercharger, jake) step = pitchFactor
- [x] Set effect voices (horn, siren, brake, shifting, reversing, indicator, coupling, wastegate, parking brake, uncoupling, sound1) step = 1.0
- [x] Compute pitchFactor in `update()`: `pitchFactor = 1.0 + (currentRpm / maxRpm) * (maxPitchFactor - 1.0)`
- [x] During STARTING state: all voices use step = 1.0
- [x] During STOPPING state: pitchFactor ramps from current value → 1.0 (time-based, ~3.6 seconds)

---

### Task 2: Diesel Knock Cylinder-Adaptive Volume

**Depends on:** Task 1 (pitch shifting infrastructure)
**Files:** `RcEngineSound.h`, `RcEngineSound.cpp`, `VehicleProfile.h`

- [x] Add `KnockPattern` enum to `RcEngineSound.h`: `V8, V8_468, R6, R6_2, V2, UNIFORM`
- [x] Add `knockPattern` and `knockAdaptiveVolume` to Config
- [x] Add `knockInterval` to Config (pulses per idle loop, default 8)
- [x] Add `curKnockCylinder` counter (1-indexed) to track position in firing sequence
- [x] Add `lastKnockTriggerSample` to track when last knock was triggered
- [x] Replace continuous knock loop with trigger-based: every `idleSampleCount / knockInterval` samples of idle, trigger knock pulse
- [x] In `getNextSample()`, when knock triggered: start playing single knock pulse from position 0
- [x] Apply cylinder-adaptive volume: loud pulses at pattern-defined positions, secondary pulses at `knockAdaptiveVolume %`
- [x] Add pattern lookup tables for V8, V8_468, R6, R6_2, V2, UNIFORM
- [x] Update `VehicleProfile::parseConfig()` to read `DIESEL_KNOCK_PATTERN`, `DIESEL_KNOCK_INTERVAL`, `KNOCK_ADAPTIVE_VOLUME` from JSON

---

### Task 3: Jake Brake Engine Slowdown

**Depends on:** Task 1
**Files:** `RcEngineSound.h`, `RcEngineSound.cpp`, `VehicleProfile.h`

- [x] Add `jakeBrakeDecelRate` to Config (default 5)
- [x] Add `jakeBrakeActive` bool to RcEngineSound class
- [x] Add auto-detection logic in `update()`: activate when RPM > jakeBrakeMinRpm AND throttle released
- [x] When jakeBrakeActive: force `currentRpm -= jakeBrakeDecelRate` (clamped to 0)
- [x] When jakeBrakeActive: set `engineMuted = true` (idle/rev/turbo/knock/fan/supercharger volumes → 0)
- [x] When throttle applied: clear engineMuted, deactivate jake brake
- [x] Jake brake voice uses pitchFactor (pitch shifts with engine)
- [x] Update `VehicleProfile::parseConfig()` to read `JAKEBRAKE_DECEL_RATE` from JSON

---

### Task 4: PARKING_BRAKE State

**Depends on:** Task 1
**Files:** `RcEngineSound.h`, `RcEngineSound.cpp`

- [x] Add `PARKING_BRAKE` to `EngineState` enum
- [x] In STOPPING state: ramp pitchFactor from current → 1.0 over ~40 steps (80ms each)
- [x] In STOPPING state: volume fade via attenuator
- [x] In STOPPING state: when attenuator >= 40, transition to PARKING_BRAKE (or OFF if no sound)
- [x] In PARKING_BRAKE state: play parking brake one-shot sound
- [x] When parking brake sound finishes: transition to OFF
- [x] Verify state machine: OFF → STARTING → RUNNING → STOPPING → PARKING_BRAKE → OFF

---

### Task 5: Automatic Transmission Simulation

**Depends on:** Task 1
**Files:** `RcEngineSound.h`, `RcEngineSound.cpp`, `VehicleProfile.h`

- [x] Add `TransmissionType` enum: `NONE, MANUAL, AUTOMATIC`
- [x] Add `transmissionType`, `numberOfGears`, `gearRampTimes[]` to Config
- [x] Add `selectedGear` and `virtualSpeed` to RcEngineSound class
- [x] In `update()`, when transmissionType == AUTOMATIC:
  - [x] Virtual speed increases/decreases slowly based on throttle
  - [x] Select gear based on virtual speed
  - [x] Map throttle to gear-limited RPM range
  - [x] Reset virtualSpeed to 0 on engine off
- [x] Apply per-gear ramp time: use `gearRampTimes[selectedGear]` for acceleration step
- [x] When transmissionType == MANUAL or NONE: RPM follows throttle directly
- [x] Update `VehicleProfile::parseConfig()` to read transmission config from JSON

---

### Task 6: Supercharger Start Point

**Depends on:** Task 1
**Files:** `RcEngineSound.h`, `RcEngineSound.cpp`, `VehicleProfile.h`

- [x] Add `superchargerStartPoint` to Config (default 10)
- [x] In supercharger voice logic: if RPM < superchargerStartPoint, volume = 0
- [x] Above start point: volume scales from 0 → 100% based on RPM
- [x] Update `VehicleProfile::parseConfig()` to read `SUPERCHARGER_START_POINT` from JSON

---

### Task 7: Uncoupling Separate Sound

**Depends on:** None
**Files:** `RcEngineSound.h`, `RcEngineSound.cpp`, `VehicleProfile.h`

- [x] Add `uncouplingSamples` and `uncouplingSampleCount` to SoundData
- [x] Add `triggerUncoupling(bool active)` method to RcEngineSound
- [x] Add uncoupling voice handling in `getNextSample()` (one-shot, like coupling)
- [x] Add `UNCOUPLING` to VehicleProfile SoundIndex enum
- [x] Update `VehicleProfile::load()` to load uncoupling sound files

---

### Task 8: Sound1 Generic Channel

**Depends on:** None
**Files:** `RcEngineSound.h`, `RcEngineSound.cpp`, `VehicleProfile.h`

- [x] Add `sound1Samples` and `sound1SampleCount` to SoundData
- [x] Add `triggerSound1(bool active)` method to RcEngineSound
- [x] Add `sound1Volume` to Config
- [x] Add sound1 voice handling in `getNextSample()` (loop while active, like horn)
- [x] Add `SOUND1` to VehicleProfile SoundIndex enum
- [x] Update `VehicleProfile::load()` to load sound1 files

---

### Task 9: ESC Ramp Time Per Gear

**Depends on:** Task 5
**Files:** `RcEngineSound.h`, `RcEngineSound.cpp`, `VehicleProfile.h`

- [x] Add `gearRampTimes[]` array to Config (sized by max gears, e.g., 6)
- [x] In automatic transmission logic, use `gearRampTimes[selectedGear]` as the acceleration step
- [x] Default ramp times: [20, 50, 75, 75, 75, 75] (first 3 gears slower response)
- [x] Update `VehicleProfile::parseConfig()` to read `GEAR_RAMP_TIMES` from JSON

---

## Testing Checklist

- [x] Build succeeds (PlatformIO compilation verified)
- [x] All 9 features implemented in RcEngineSound.h/cpp
- [x] VehicleProfile.h updated with new config parsing
- [x] vehicle-config.json updated with new parameters
- [x] Code reviewed for correctness (3 review rounds)
- [ ] Verify pitch shifting: engine sound pitch rises/falls with RPM (on device)
- [ ] Verify knock: V8 pattern produces correct loud/quiet pulse rhythm (on device)
- [ ] Verify jake brake: activates on throttle release above min RPM (on device)
- [ ] Verify PARKING_BRAKE: shutdown sequence completes with air brake sound (on device)
- [ ] Verify auto-trans: RPM drops at gear shift points (on device)
- [ ] Profile CPU usage: ISR completes within budget at 8 concurrent voices (on device)
