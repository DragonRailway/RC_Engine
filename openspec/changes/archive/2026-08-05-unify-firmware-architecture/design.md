## Context

The RC Brain codebase previously maintained two separate main entry points (`RC_Loco/src/main.cpp` and `RC_Truck/src/main.cpp`) and two PlatformIO environments (`[env:RC_Loco]` and `[env:RC_Truck]`). Vehicle type was determined at compile time using build flags (`-D RC_TRUCK` and `-D RC_LOCO`). 

To eliminate code duplication, streamline firmware builds, and enable dynamic hardware/vehicle configuration on the target device, we are consolidating into a single firmware binary (`src/main.cpp`) targeting `[env:TRACKLINK_V3]` in `platformio.ini`.

## Goals / Non-Goals

**Goals:**
- Consolidate firmware source into a single `src/main.cpp` entrypoint.
- Remove duplicate `RC_Loco/` and `RC_Truck/` folders.
- Simplify `platformio.ini` to a single build environment (`[env:TRACKLINK_V3]`).
- Extend `ConfigParser` to parse `VEHICLE.TYPE` dynamically from `/vehicle-config.json` into a runtime enum (`VehicleType::TRUCK`, `VehicleType::LOCOMOTIVE`, etc.).
- Maintain robust LittleFS config loading with fallback sound resolution (`/sounds/{NAME}-{type}.json` -> `/sounds/{type}.json`).
- Implement boot safety so missing or corrupt config files log a fatal error and halt outputs safely.

**Non-Goals:**
- Creating a dynamic web configuration interface or Bluetooth/WiFi live config editor in this change.
- Altering the audio synthesis PCM pipeline format (remains 22,050 Hz, 8-bit PCM).

## Decisions

### Decision 1: Canonical Config Loading (Option B)
- **Choice**: The unified firmware will directly read `/hardware-config.json` and `/vehicle-config.json` at boot from the root of LittleFS.
- **Rationale**: Single-vehicle deployment does not require an extra pointer file (e.g. `system.json`). Canonical paths keep LittleFS file structures simple, deterministic, and easy to flash.
- **Alternatives Considered**: Pointer manifest file (`/system.json`), which adds extra filesystem lookups and dangling pointer risks.

### Decision 2: Runtime `VehicleType` Enum
- **Choice**: Add `VehicleType` enum to `RcEngineSound::Config` or `Config.h`, populated from `doc["VEHICLE"]["TYPE"]` string.
- **Rationale**: Removes preprocessor macros (`-D RC_TRUCK`, `-D RC_LOCO`), allowing the same compiled binary to run any vehicle profile loaded onto LittleFS.
- **Alternatives Considered**: Compile-time build flags, which require rebuilding and re-flashing binary per vehicle type.

### Decision 3: Directory Cleanup
- **Choice**: Move main entry logic to `src/main.cpp` at repository root level, delete `RC_Loco/` and `RC_Truck/` subdirectories.
- **Rationale**: Keeps repository structure aligned with standard PlatformIO project layouts and removes redundant code files.

## Risks / Trade-offs

- **[Risk] Missing or Corrupted Config on Flash**: If LittleFS is empty or missing `/vehicle-config.json`, the board cannot determine hardware pins or engine sound profiles.
  - **Mitigation**: Detect load failure in `setup()`, log a clear fatal message via 2,000,000 baud Serial, and keep all PWM and audio pins uninitialized/disabled.

- **[Risk] Sound File Name Mismatch**: If `vehicle-config.json` specifies a custom vehicle name without matching `/sounds/{NAME}-{type}.json` files.
  - **Mitigation**: `ConfigParser::loadSounds()` falls back to generic sound files (`/sounds/{type}.json` or standard generic names) if prefix-matched sound JSONs are missing.

## Migration Plan

1. Create `src/main.cpp` incorporating consolidated initialization logic.
2. Update `ConfigParser.h` to read `VEHICLE.TYPE` and populate runtime enum.
3. Simplify `platformio.ini` to default to `src_dir = src` and `[env:TRACKLINK_V3]`.
4. Delete `RC_Loco/` and `RC_Truck/` directories.
5. Verify build with `pio run` and verify test suite with `pio test`.
