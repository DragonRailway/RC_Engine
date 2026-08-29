## Why

`SoundData` allocations via `ps_malloc()` are never freed on hot-reload. When a vehicle name changes during tuning, the old PSRAM buffers (~460KB–4.1MB per vehicle) are silently leaked because `isDynamic` is never set to `true` and `SoundData` has no cleanup path. The destructor in `RcEngineSound` already has free logic gated on `isDynamic`, but the flag is never set. Additionally, all three core infrastructure headers (`VehicleController.h`, `HardwareInit.h`, `ConfigParser.h`) are header-only with ~190 static member variable definitions inline, creating ODR violations when included from multiple translation units (both `src/main.cpp` and `test/host_vc/host_vc_driver.cpp` include them).

## What Changes

- **Sound memory lifecycle**: `SoundData` gains a `clear()` method, an assignment operator with cleanup, and `isDynamic = true` is set after allocation. Every `SoundData = SoundData()` now automatically frees old PSRAM buffers before overwriting.
- **Header/.cpp split**: `VehicleController.h` (1195→~80 lines), `HardwareInit.h` (895→~80 lines), and `ConfigParser.h` (1135→~50 lines) are split into declaration headers + implementation `.cpp` files in `common/`.
- **Build system**: PlatformIO `src_filter` updated to compile `common/*.cpp`. Host test script updated to link the new `.cpp` files.

## Capabilities

### New Capabilities
- `sound-memory-lifecycle`: PSRAM allocation tracking, automatic cleanup on `SoundData` reassignment, and safe hot-reload of sound assets without memory leaks.

### Modified Capabilities
_(none — the header split is a structural refactor with no behavioral spec changes)_

## Impact

- **Files modified**: `lib/SoundEngine/src/SoundTypes.h`, `lib/SoundEngine/src/RcEngineSound.cpp`, `common/ConfigParser.h` (trimmed), `common/VehicleController.h` (trimmed), `common/HardwareInit.h` (trimmed), `platformio.ini` (3 envs), `scripts/host_vc_test.py`
- **Files created**: `common/VehicleController.cpp`, `common/HardwareInit.cpp`, `common/ConfigParser.cpp`
- **Files removed**: `lib/SoundEngine/src/SoundLoader.h` (empty vestigial stub)
- **Build system**: PlatformIO `src_filter` addition to all 3 board envs; host test g++ source list updated
- **Risk**: Medium — header split touches build system and every file that includes the three headers. Mitigated by the existing 25-test host test suite as regression gate.
