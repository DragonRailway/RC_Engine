## Context

The RC Engine firmware boots on ESP32-S3 and initializes LittleFS, RadioKit (BLE + FS protocol), hardware peripherals, and the sound engine. Two independent codepaths currently mount LittleFS:

1. `ConfigParser::begin()` calls `LittleFS.begin(true)` directly at `common/ConfigParser.cpp:47`
2. `RadioKit.enableFS()` calls `RKFs::begin()` at `lib/rk-arduino/src/RadioKit.cpp:1457`, which internally calls `LittleFS.begin()`

The `RKFs::begin()` function (in `RadioKitFsHandlers.cpp:141-165`) has a probe workaround: if `LittleFS.open("/")` succeeds, it sets `s_mounted = true` without reinitializing. This prevents a double-mount crash but means the mount state is corrected retroactively.

Additionally, the current `RADIOKIT.h` starts BLE before mounting FS:

```
RadioKit.begin();
RadioKit.startSerial(Serial);
RadioKit.startBLE();        ← BLE starts first
RadioKit.enableFS();        ← FS mounts after
```

The new RadioKit library on `multi-ui` branch has been updated with boot flush logic and the safe init order. RC_brain needs to pull these changes and align its init sequence.

## Goals / Non-Goals

**Goals:**
- Centralize LittleFS mounting through `RKFs::begin()` so `s_mounted` is correct from the first mount
- Ensure `enableFS()` is called before `startBLE()` in the RadioKit init sequence
- Pull the latest RadioKit library with boot flush and safe init fixes
- Maintain identical runtime behavior — same configs loaded, same sounds, same BLE/FS protocol

**Non-Goals:**
- Refactoring ConfigParser beyond the mount call (the rest of the parser is untouched)
- Changing the RadioKit library API or behavior
- Adding new capabilities or features
- Modifying the `setup()` function's overall structure beyond the init order fix

## Decisions

### Decision 1: Re-fetch library via existing script

**Choice**: Run `rm -rf lib/rk-arduino && python3 scripts/fetch_radiokit.py` to pull the latest `multi-ui` branch.

**Why**: The fetch script (`scripts/fetch_radiokit.py`) already handles sparse-checkout from `Radio-Kit/RadioKit` on the `multi-ui` branch. It clones to a temp directory, copies `rk-arduino/` into `lib/`, and cleans up. No manual intervention needed.

**Alternative considered**: Git submodule — rejected because the project already uses a vendored copy fetched by script, and adding submodule management would be a larger change.

### Decision 2: ConfigParser uses RKFs::begin() instead of LittleFS.begin()

**Choice**: Replace the direct `LittleFS.begin(true)` call in `ConfigParser::begin()` with `RKFs::begin()` and check `RKFs::isReady()` for the error path.

**Why**: This centralizes mount state. After this change, there is exactly one codepath that mounts LittleFS — `RKFs::begin()`. The `s_mounted` flag is set on the first mount, and subsequent calls (from `enableFS()` in `initRadioKit()`) short-circuit immediately.

**Code change** (conceptual):
```cpp
// Before:
bool ConfigParser::begin() {
    if (!LittleFS.begin(true)) {
        Serial.println("[ConfigParser] LittleFS mount failed");
        return false;
    }
    Serial.println("[ConfigParser] LittleFS mounted");
    return true;
}

// After:
bool ConfigParser::begin() {
    RKFs::begin();
    if (!RKFs::isReady()) {
        Serial.println("[ConfigParser] LittleFS mount failed");
        return false;
    }
    Serial.println("[ConfigParser] LittleFS mounted");
    return true;
}
```

**Alternative considered**: Leave ConfigParser calling `LittleFS.begin()` directly and rely on the probe — rejected because it leaves mount state fragmented and order-dependent.

### Decision 3: RADIOKIT.h init order — FS before BLE

**Choice**: Regenerate or manually edit `src/RADIOKIT.h` so the init sequence is:

```
RadioKit.begin();
RadioKit.startSerial(Serial);
RadioKit.enableFS();        ← FS mounts first (safe)
RadioKit.startBLE();        ← BLE starts after
```

**Why**: LittleFS must be mounted before BLE starts to avoid flash DMA conflicts on ESP32-S3 with embedded XMC flash. The new codegen from the RadioKit app produces this order. Manual edit achieves the same result.

**Alternative considered**: Use the RadioKit app Designer to regenerate — same result, but manual edit is faster for a two-line swap.

### Decision 4: Fix ordering

**Choice**: Apply fixes in order: library re-fetch → RADIOKIT.h update → ConfigParser refactor.

**Why**: The library re-fetch must come first because both the RADIOKIT.h update and the ConfigParser refactor depend on the new `RKFs::begin()` behavior. The RADIOKIT.h update is independent of ConfigParser — either one alone prevents the worst-case scenario (BLE before FS with no probe). The ConfigParser refactor is last because it's the optional hygiene fix.

## Risks / Trade-offs

- **[Risk] Library re-fetch overwrites local changes** → The `lib/rk-arduino/` directory is fully replaced. No local modifications exist in the vendored copy, so this is safe. The fetch script deletes and recreates the directory.
- **[Risk] ConfigParser refactor changes error behavior** → `RKFs::begin()` returns false if LittleFS fails, same as `LittleFS.begin(true)`. The error path is identical. `RKFs::isReady()` checks `s_mounted`, which is set by `LittleFS.begin()` internally.
- **[Trade-off] ConfigParser now depends on RKFs** → Adds a dependency on the RadioKit library's FS module. This is acceptable because ConfigParser already depends on LittleFS, and RKFs is the intended wrapper around LittleFS in this project.
- **[Risk] Manual RADIOKIT.h edit vs. codegen** → A manual edit could be overwritten by the next codegen export. This is acceptable because the codegen now produces the correct order, and the manual edit matches it.
