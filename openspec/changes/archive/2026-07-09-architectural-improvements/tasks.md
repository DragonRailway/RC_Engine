# Tasks: Architectural Improvements

## Phase 1: Bug Fixes (High Priority)

- [x] **Task 1: Fix DC offset bug in AudioOutput.h**
  - Changed `audioTask()` offset ramp to scale buffer (multiply by 0→1.0)
  - Fixed volatile warning: `bufferPos++` → `bufferPos = bufferPos + 1`

- [x] **Task 2: Fix ISR/main loop thread safety**
  - Added `portMUX_TYPE voiceMutex` to RcEngineSound
  - `getNextSample()` snapshots voices under critical section
  - Only writes back `position` (not `active`/`volume`) to avoid race with `update()`

## Phase 2: Data Structure Refactor (Medium Priority)

- [x] **Task 3: Create shared SoundTypes.h**
  - Defined `SoundID` enum (24 types + `SOUND_COUNT`)
  - Defined `SoundSlot` and `SoundData` structs

- [x] **Task 4: Refactor SoundData to indexed array**
  - Replaced 48 named fields with `SoundSlot slots[SOUND_COUNT]`
  - Removed unused `hornSampleRate` field

- [x] **Task 5: Unify VoiceID and SoundIndex**
  - Removed `VoiceID` and `SoundIndex` enums
  - All code uses shared `SoundID` from SoundTypes.h

- [x] **Task 6: Update begin() to use loops**
  - Replaced 80 lines with VoiceDef lookup table + loop
  - Destructor loops over `SOUND_COUNT`
  - Volume assignment via lookup

- [x] **Task 7: Update SoundLoader to use SoundSlot**
  - Returns `SoundSlot*` instead of `SoundData*`
  - `unloadAll()` loops over `SOUND_COUNT`

- [x] **Task 8: Update VehicleProfile to use indexed SoundData**
  - `sounds[SOUND_COUNT]` array of `SoundSlot*`
  - `populateSoundData()` copies slots in loop
  - Uses `SoundID` from SoundTypes.h

## Phase 3: Config Refactor (Medium Priority)

- [x] **Task 9: Group Config into sub-structs**
  - Created `Config::Engine`, `Config::Sound`, `Config::Transmission`, `Config::Features`, `Config::LoopPoints`
  - Removed legacy fields (`automatic`, `clutchEngagingPoint`, `maxRpmPercentage`)

- [x] **Task 10: Update VehicleProfile::parseConfig()**
  - Updated to parse into sub-structs
  - Removed legacy field parsing

- [x] **Task 11: Update all Config field references in RcEngineSound.cpp**
  - All `cfg.field` → `cfg.sub.field` updated
  - Removed all `VOICE_*` enum references (now using `SoundID` directly)

## Phase 4: Validation

- [x] **Task 12: Build and verify compilation**
  - Build succeeds at 7.6% RAM, 13.4% Flash
  - No missed references, no new warnings

- [x] **Task 13: Code review**
  - Fixed one-shot voice deactivation (write back `active` flag)
  - Removed dead `clutchEngagingPoint` field
  - Verified no external references to removed `Lights` sub-struct
  - All reviews passed
