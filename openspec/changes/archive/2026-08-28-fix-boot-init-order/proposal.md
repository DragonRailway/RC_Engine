## Why

The current boot sequence mounts LittleFS in two places: `ConfigParser::begin()` calls `LittleFS.begin()` directly, then `RadioKit.enableFS()` calls `RKFs::begin()` which probes whether FS is already mounted and retroactively sets `s_mounted = true`. A probe workaround in `RKFs::begin()` prevents a double-mount crash, but the mount state is healed after the fact rather than correct from the start. Additionally, the current `RADIOKIT.h` codegen starts BLE before calling `enableFS()`, which means the filesystem isn't ready when BLE begins — the probe saves us, but this is fragile and order-dependent.

## What Changes

- **Re-fetch RadioKit library** from GitHub `multi-ui` branch to pull in `_bootFlushDone` flag, boot flush logic in `_flushPrintBuffer()`, and the `RKFs::begin()` probe workaround that detects pre-mounted LittleFS.
- **Update `RADIOKIT.h` init order** so `enableFS()` is called before `startBLE()`, matching the new codegen output. This ensures the filesystem is mounted and ready before the BLE stack starts.
- **Refactor `ConfigParser::begin()`** to use `RKFs::begin()` instead of `LittleFS.begin(true)` directly, centralizing mount state tracking. The `s_mounted` flag in `RKFs` will be correct from the first mount, not retroactively patched.

## Capabilities

### New Capabilities

<!-- None — this is a fix to existing behavior, not a new capability -->

### Modified Capabilities

- `unified-firmware-entrypoint`: The init sequence requirement changes — LittleFS MUST be mounted through `RKFs::begin()` (not directly via `LittleFS.begin()`), and `enableFS()` MUST be called before `startBLE()` in the RadioKit init sequence.

## Impact

- **Files modified**: `src/RADIOKIT.h` (init order), `common/ConfigParser.cpp` (mount call), `lib/rk-arduino/` (re-fetched library)
- **Boot sequence**: The order of operations in `setup()` changes — `ConfigParser::begin()` now uses `RKFs::begin()`, and `initRadioKit()` mounts FS before starting BLE
- **No API changes**: Public interfaces (`ConfigParser::begin()`, `RadioKit.enableFS()`) keep the same signatures
- **No config changes**: Hardware and vehicle configs are unaffected
- **Risk**: Low — same effective behavior, cleaner state tracking. The probe workaround already proves the code path works; this just makes it the intended path instead of a fallback.
