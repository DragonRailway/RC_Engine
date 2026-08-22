## Context

In `RC_brain`, device identity is dynamically driven by configuration bundles on LittleFS (`hardware-<BOARD>.json` and `vehicle-config.json`). `applyDeviceMetadata()` is called during boot (`setup()`) and on config file updates (`reloadConfigs()`) to set `RadioKit.config.name`, `RadioKit.config.description`, and `RadioKit.config.type`.

In `RadioKit`, the library maintains an NVS cache (`_nvsName`) to allow user-renaming from the mobile app settings. When `RadioKit.begin()` runs, `_syncNvsToBuffers()` reads the existing NVS string (e.g. `"RC_UI"`), which takes precedence over `RadioKit.config.name` in `_buildConfPayload()` and BLE advertising.

## Goals / Non-Goals

**Goals:**
- Ensure `applyDeviceMetadata()` in `src/main.cpp` calls `RadioKit.setName(targetName)` so that `_nvsName`, `CONF_DATA`, and live BLE advertisements immediately match the resolved 3-tier cascade (`Hardware > Vehicle > Fallback`).
- Keep BLE advertising name synchronized dynamically on boot and upon LittleFS config hot-reloads.

**Non-Goals:**
- Modifying internal RadioKit NVS storage keys or transport protocol byte structures.
- Erasing user calibration or network preferences stored in NVS.

## Decisions

### Decision 1: Explicitly invoke `RadioKit.setName(targetName)` in `applyDeviceMetadata()`
- **Rationale**: `RadioKitClass::setName(const char* name)` updates `_nvsName`, writes the value to NVS if changed, and calls `_setBleAdvertisingName()`. Calling this inside `applyDeviceMetadata()` guarantees that the active vehicle name is always committed to the NVS buffer and BLE stack.
- **Alternatives considered**:
  - *Erasing NVS partition via flash tools*: Only works during initial flashing; does not fix hot-reloads or switching vehicle bundles on the filesystem.
  - *Modifying RadioKit library to ignore NVS*: Breaks standard RadioKit applications that rely on mobile app renaming.

## Risks / Trade-offs

- **[Risk]** Overwrites in-app device renames on firmware boot or vehicle config reload.
  - *Mitigation*: In RC Brain, vehicle profile identity is intentionally file-driven by `vehicle-config.json`. Overriding device name to match the selected model is the expected behavior.
