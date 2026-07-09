# Parity Gap Closures — Tasks

## Implementation Order

Tasks are ordered by dependency. Loop points are the foundation for feature 1; the rest are independent.

---

### Task 1: Horn/Siren/Reversing Loop Points

**Depends on:** None
**Files:** `RcEngineSound.h`, `RcEngineSound.cpp`, `VehicleProfile.h`

- [x] Add `loopBegin` and `loopEnd` fields (uint32_t) to `VoiceState` struct in `RcEngineSound.h`
- [x] Modify `advanceVoice()` to respect loop region when `loopEnd > 0`
- [x] Add `loopBegin`/`loopEnd` to `Config` struct for horn, siren, reversing, sound1
- [x] In `begin()`, set loopBegin/loopEnd on VOICE_HORN, VOICE_SIREN, VOICE_REVERSING, VOICE_SOUND1 from config
- [x] Update `VehicleProfile::parseConfig()` to read `HORN_LOOP_BEGIN`, `HORN_LOOP_END`, `SIREN_LOOP_BEGIN`, `SIREN_LOOP_END`, `REVERSING_LOOP_BEGIN`, `REVERSING_LOOP_END`, `SOUND1_LOOP_BEGIN`, `SOUND1_LOOP_END` from JSON
- [x] Default all loop points to 0 (backward compatible — full sample plays)

---

### Task 2: DAC Offset Fade (Pop Prevention)

**Depends on:** None
**Files:** `AudioOutput.h`

- [x] Add `offsetRamping` bool and `currentOffset` uint8_t to `AudioOutput` class
- [x] In `audioTask()`, ramp `currentOffset` from 0→128 over first ~276 samples (12ms at 22050Hz)
- [x] Add `offsetRamping = false` in `stop()` for re-initialization on engine restart

---

### Task 3: RPM-Dependent Knock Volume

**Depends on:** None
**Files:** `RcEngineSound.h`, `RcEngineSound.cpp`, `VehicleProfile.h`

- [x] Add `minKnockVolume` (uint8_t, default 80%) and `knockStartRpm` (uint8_t, default 10%) to Config
- [x] In `update()`, replace fixed knock volume calculation with RPM-scaled version
- [x] Linear map: at `knockStartRpm` → `minKnockVolume%`, at `maxRpm` → 100%
- [x] Apply to both loud and secondary knock pulses
- [x] Update `VehicleProfile::parseConfig()` to read `MIN_KNOCK_VOLUME`, `KNOCK_START_RPM` from JSON

---

### Task 4: Manual Transmission Shifting Trigger

**Depends on:** None
**Files:** `RcEngineSound.h`, `RcEngineSound.cpp`

- [x] Add `lastGear` uint8_t to RcEngineSound class (initialized to 1)
- [x] In `update()`, when `transmissionType == TRANS_MANUAL && state == RUNNING`:
  - Compute gear from `targetRpm / gearSize`
  - Compare with `lastGear`
  - If changed: trigger VOICE_SHIFTING (active=true, position=0)
  - Update `lastGear` and `selectedGear`

---

### Task 5: Voice Mixing Weights

**Depends on:** None
**Files:** `RcEngineSound.h`, `RcEngineSound.cpp`, `VehicleProfile.h`

- [x] Add `engineMixWeight` and `effectMixWeight` (uint8_t, default 100) to Config
- [x] In `getNextSample()`, accumulate engine voices and effect voices separately
- [x] Apply weights: `engineMix * engineMixWeight / 100` and `effectMix * effectMixWeight / 100`
- [x] Sum weighted groups, apply master volume and DC offset
- [x] Update `VehicleProfile::parseConfig()` to read `ENGINE_MIX_WEIGHT`, `EFFECT_MIX_WEIGHT` from JSON

---

### Task 6: Crawler Mode

**Depends on:** None
**Files:** `RcEngineSound.h`, `RcEngineSound.cpp`, `VehicleProfile.h`

- [x] Add `crawlerModeThreshold` (uint8_t, default 44) to Config
- [x] Add `crawlerMode` bool to RcEngineSound class
- [x] In `update()`, compute `crawlerMode = (cfg.masterVolume <= cfg.crawlerModeThreshold)`
- [x] When `crawlerMode == true`: set `currentRpm = effectiveTarget` (instant response, skip inertia)
- [x] When `crawlerMode == false`: use normal inertia calculation
- [x] Update `VehicleProfile::parseConfig()` to read `CRAWLER_MODE_THRESHOLD` from JSON

---

### Task 7: Update Vehicle Config JSONs

**Depends on:** Tasks 1-6
**Files:** `RC_Truck/configs/vehicle-config.json`, `RC_Truck/configs/vehicle-ScaniaV8.json`

- [x] Add `LOOP_POINTS` section with all loop begin/end values
- [x] Add `KNOCK_START_RPM` and `MIN_KNOCK_VOLUME` to engine section
- [x] Add `ENGINE_MIX_WEIGHT` and `EFFECT_MIX_WEIGHT` to sound volume section
- [x] Add `CRAWLER_MODE_THRESHOLD` to sound volume section
- [x] Verify all new parameters have sensible defaults for ScaniaV8

---

## Testing Checklist

- [x] Build succeeds (PlatformIO compilation verified)
- [ ] Horn loops correctly when trigger held (on device)
- [ ] Siren loops correctly when trigger held (on device)
- [ ] Reversing beep loops continuously while reversing (on device)
- [ ] No audio pop on startup (on device)
- [ ] Knock volume is quieter at idle, louder at high RPM (on device)
- [ ] Manual transmission triggers shifting sound on gear change (on device)
- [ ] No clipping when many effects play simultaneously (on device)
- [ ] Crawler mode provides instant RPM response at low volume (on device)
