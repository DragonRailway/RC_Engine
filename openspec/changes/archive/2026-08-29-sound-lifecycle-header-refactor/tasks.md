## 1. Sound Memory Lifecycle Fix

- [x] 1.1 Add `SoundData::clear()` method to `lib/SoundEngine/src/SoundTypes.h` — frees PSRAM buffers for all slots when `isDynamic == true`, resets slots to default
- [x] 1.2 Add `SoundData::operator=(const SoundData&)` to `SoundTypes.h` — calls `clear()` before copying, includes self-assignment check
- [x] 1.3 Add `SoundData::operator=(SoundData&&)` move assignment to `SoundTypes.h` — transfers ownership, clears source
- [x] 1.4 Set `isDynamic = true` in `ConfigParser::loadSounds()` after successful `ps_malloc()` allocation, in `common/ConfigParser.h`
- [x] 1.5 Add `sounds.clear()` before `sounds = soundData` in `RcEngineSound::begin()` in `lib/SoundEngine/src/RcEngineSound.cpp`
- [x] 1.6 Run `python3 scripts/host_vc_test.py` — all 25 tests must pass

## 2. VehicleController Header Split

- [x] 2.1 Create `common/VehicleController.cpp` with all static member variable definitions (lines 1124–1195 of current header) and move all non-inline method implementations
- [x] 2.2 Trim `common/VehicleController.h` to declarations only — keep class definition with method signatures, remove method bodies except trivial one-liners
- [x] 2.3 Add `#include "VehicleController.h"` to `common/VehicleController.cpp`
- [x] 2.4 Run `python3 scripts/host_vc_test.py` — all 25 tests must pass

## 3. HardwareInit Header Split

- [x] 3.1 Create `common/HardwareInit.cpp` with all static member variable definitions (lines 850–895 of current header) and move all non-inline method implementations
- [x] 3.2 Trim `common/HardwareInit.h` to declarations only — keep class definition with method signatures, remove method bodies except trivial one-liners
- [x] 3.3 Add `#include "HardwareInit.h"` to `common/HardwareInit.cpp`
- [x] 3.4 Run `python3 scripts/host_vc_test.py` — all 25 tests must pass

## 4. ConfigParser Header Split

- [x] 4.1 Create `common/ConfigParser.cpp` with static member definitions and move all non-inline method implementations
- [x] 4.2 Trim `common/ConfigParser.h` to declarations only — keep class definition with method signatures
- [x] 4.3 Add `#include "ConfigParser.h"` to `common/ConfigParser.cpp`
- [x] 4.4 Run `python3 scripts/host_vc_test.py` — all 25 tests must pass

## 5. Build System Updates

- [x] 5.1 Add `src_filter = +<*> +<../common/>` to all 3 env sections in `platformio.ini` (TRACKLINK_V3, MIKRO_V2, GTRACK)
- [x] 5.2 Add `common/VehicleController.cpp`, `common/HardwareInit.cpp`, `common/ConfigParser.cpp` to g++ source list in `scripts/host_vc_test.py`
- [x] 5.3 Run `pio run -e TRACKLINK_V3` — build must succeed
- [x] 5.4 Run `pio run -e MIKRO_V2` — build must succeed
- [x] 5.5 Run `pio run -e GTRACK` — build must succeed
- [x] 5.6 Run `python3 scripts/host_vc_test.py` — all 25 tests must pass

## 6. Cleanup

- [x] 6.1 Delete `lib/SoundEngine/src/SoundLoader.h` (empty vestigial stub)
- [x] 6.2 Verify no file includes `SoundLoader.h` — grep for `#include.*SoundLoader`
- [x] 6.3 Run full host test suite: `python3 scripts/host_vc_test.py` and `python3 scripts/host_dsp_test.py`
