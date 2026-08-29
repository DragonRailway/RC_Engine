## 1. Re-fetch RadioKit Library

- [x] 1.1 Remove the stale `lib/rk-arduino/` directory
- [x] 1.2 Run `python3 scripts/fetch_radiokit.py` to pull latest `multi-ui` branch
- [x] 1.3 Verify `lib/rk-arduino/src/RadioKitFsHandlers.cpp` contains the `s_mounted` probe logic (lines 141-165)
- [x] 1.4 Verify `lib/rk-arduino/src/RadioKitClass.h` contains `_bootFlushDone` flag

## 2. Update RADIOKIT.h Init Order

- [x] 2.1 In `src/RADIOKIT.h`, swap lines so `RadioKit.enableFS()` is called before `RadioKit.startBLE()`
- [x] 2.2 Verify the init sequence in `initRadioKit()` is: `begin()` → `startSerial()` → `enableFS()` → `startBLE()`

## 3. Refactor ConfigParser to Use RKFs

- [x] 3.1 In `common/ConfigParser.cpp`, replace `LittleFS.begin(true)` in `ConfigParser::begin()` with `RKFs::begin()`
- [x] 3.2 Replace the `!LittleFS.begin(true)` error check with `!RKFs::isReady()`
- [x] 3.3 Add `#include <RadioKitLib.h>` or appropriate RKFs header if not already included

## 4. Build and Verify

- [x] 4.1 Run `pio run -e TRACKLINK_V3` and confirm clean build (no warnings/errors)
- [x] 4.2 Run `pio run -e MIKRO_V2` and confirm clean build (if MIKRO_V2 env exists)
- [x] 4.3 Flash to hardware and verify boot sequence: LittleFS mounts before BLE starts, no double-mount warnings, configs load correctly
