## Context

The RC Engine firmware uses a header-only architecture for its three core infrastructure classes: `VehicleController.h` (1195 lines), `HardwareInit.h` (895 lines), and `ConfigParser.h` (1135 lines). Static member variable definitions are placed inline in headers, creating ODR violations when headers are included from multiple translation units. Additionally, `SoundData` allocates PSRAM buffers via `ps_malloc()` but never frees them on hot-reload, leaking ~460KB–4.1MB per vehicle name change.

Current state:
- `SoundData` has `isDynamic` flag but it's never set to `true`
- `RcEngineSound::~RcEngineSound()` has free logic gated on `isDynamic` — already correct
- `ConfigParser::loadSoundSlot()` allocates with `ps_malloc()` — no tracking
- `VehicleController.h`, `HardwareInit.h`, `ConfigParser.h` have ~190 static member definitions inline
- Both `src/main.cpp` and `test/host_vc/host_vc_driver.cpp` include these headers — two TUs, ODR violation

## Goals / Non-Goals

**Goals:**
- Fix PSRAM memory leak on sound hot-reload
- Make `SoundData` self-managing (RAII-style cleanup on assignment/destruction)
- Eliminate ODR violations by moving static member definitions to `.cpp` files
- Maintain all 25 existing host VC tests as regression gate
- Keep the header split minimal — declarations in `.h`, definitions in `.cpp`

**Non-Goals:**
- Refactoring class APIs or renaming methods
- Changing the Arduino single-TU build model
- Adding new features or capabilities beyond memory safety
- Touching the RadioKit or ESP32_EasyKit libraries

## Decisions

### D1: SoundData cleanup via assignment operator (not manual free calls)

**Decision**: Add `SoundData::clear()` and `SoundData::operator=` to `SoundTypes.h`. The assignment operator calls `clear()` before copying.

**Why**: Every `SoundData = SoundData()` site automatically gets cleanup without changing call sites. The existing `soundData = SoundData()` in `loadSounds()` (line 294) will now properly free old buffers before resetting. No need to add explicit `freeSounds()` calls at each reload path.

**Alternatives considered**:
- *Explicit `freeSounds()` function*: Requires remembering to call it at every reload site. Miss one and the leak returns. Rejected.
- *Smart pointer wrappers*: Overkill for embedded PSRAM. Adds complexity without proportional benefit. Rejected.

### D2: Set `isDynamic = true` in ConfigParser after successful allocation

**Decision**: In `loadSoundSlot()`, after `ps_malloc()` succeeds, the calling code sets `isDynamic = true` on the `SoundData`.

**Why**: The allocation happens per-slot inside `loadSoundSlot()`, but the `isDynamic` flag lives on `SoundData`. The simplest approach is to set it once after all slots are loaded, in `loadSounds()`.

### D3: Belt-and-suspenders cleanup in RcEngineSound::begin()

**Decision**: Add `sounds.clear()` before `sounds = soundData` in `RcEngineSound::begin()`.

**Why**: Defense in depth. Even if the assignment operator handles it, an explicit cleanup before assignment makes the intent clear and protects against edge cases (e.g., self-assignment with different const-ness).

### D4: Header split — move static definitions to new .cpp files in common/

**Decision**: Create `common/VehicleController.cpp`, `common/HardwareInit.cpp`, `common/ConfigParser.cpp`. Each contains the static member variable definitions and all method implementations. Headers retain only declarations, inline trivial methods, and class definition.

**Why**: Keeps files in the same directory. The `-I common` include path already works. PlatformIO `src_filter = +<*> +<../common/>` compiles them. Host test g++ command gets the new .cpp files added.

**Alternatives considered**:
- *Move .cpp to src/*: Splits a module across directories. Messy.
- *Keep header-only, use `inline` keyword*: C++17 `inline` variables solve ODR but don't fix the 1000+ line headers. Kicks the can.

### D5: Remove vestigial SoundLoader.h

**Decision**: Delete `lib/SoundEngine/src/SoundLoader.h` (empty stub with a comment).

**Why**: It's dead code. The comment says "SoundLoader has been absorbed into ConfigParser." Removing it reduces confusion.

## Risks / Trade-offs

**[Risk] Host test breaks after header split** → The host test g++ command explicitly lists source files. New .cpp files must be added to `scripts/host_vc_test.py`. Mitigated by running `python3 scripts/host_vc_test.py` after each change.

**[Risk] PlatformIO doesn't pick up common/*.cpp** → The `src_filter` directive must be added correctly. Mitigated by testing with `pio run -e TRACKLINK_V3` before committing.

**[Risk] Double-free in SoundData operator=** → Self-assignment (`a = a`) could free then copy from freed memory. Mitigated by adding a self-assignment check: `if (this == &other) return *this;`

**[Trade-off] Header becomes thinner but .cpp becomes larger** → The total line count doesn't change. Readability improves because headers show the API surface, not the implementation.

**[Trade-off] One TU constraint for static definitions** → The `.cpp` files must be compiled into exactly one TU. This is already the case (Arduino single-TU, host test single-TU). If a second TU ever includes the `.cpp`, linker will error with duplicate symbols — fail-fast, not silent corruption.
