## Context

RadioKit exposes device metadata (`name` and `description`) over BLE and Serial during connection handshake and BLE advertising. Currently, the device name and description are statically defined in `RADIOKIT.h` (`initRadioKit()`). Users require a flexible, hierarchical fallback priority to configure name and description:
1. `hardware config JSON` (`hardware-<BOARD>.json` top-level `name` / `description`)
2. `vehicle config JSON` (`vehicle.json` `vehicle.name` / `vehicle.description`)
3. `RADIOKIT.h` (`initRadioKit()` defaults)

## Goals / Non-Goals

**Goals:**
- Extend `HardwareConfig` struct to store `char name[32]` and `char description[128]`.
- Extend `RcEngineSound::Config` in `lib/SoundEngine` to store `char description[128]` (in addition to existing `name[32]`).
- Update `ConfigParser` to parse top-level `name` and `description` from hardware JSON and `description` from `vehicle` object in vehicle JSON.
- Implement the resolution cascade in `src/main.cpp` before RadioKit start and on dynamic hot-reload.
- Update `configs/schemas/hardware_config.schema.json` to allow top-level `name` and `description`.
- Dynamically update BLE advertising name when configs hot-reload via LittleFS.

**Non-Goals:**
- Modifying RadioKit's protocol or upstream transport internals.
- Writing to NVS on config load (rely on runtime config assignment so LittleFS JSON files remain the single source of truth).

## Decisions

### Decision 1: Top-level keys for hardware config
Top-level `"name"` and `"description"` in `hardware-<BOARD>.json` will be supported (e.g. `{"name": "...", "description": "...", "sound": { ... }}`).
*Rationale*: Matches the schema structure of other top-level blocks in `hardware_config.schema.json`.

### Decision 2: Fallback cascade implementation in `src/main.cpp`
A helper function `applyDeviceMetadata(const HardwareConfig& hw, const RcEngineSound::Config& vc)` in `src/main.cpp` evaluates:
- If `hw.name[0] != '\0'` -> `RadioKit.config.name = hw.name`
- Else if `vc.name[0] != '\0' && strcmp(vc.name, "Unknown") != 0` -> `RadioKit.config.name = vc.name`
- Else -> retain `RADIOKIT.h` default
And similarly for `description`.
*Rationale*: Clean separation; `HardwareConfig` and `VehicleProfile` are global objects in RAM whose char array buffers persist indefinitely for `const char*` pointer assignment.

### Decision 3: Hot reload support
When `reloadConfigs()` is executed on file changes in LittleFS, `applyDeviceMetadata()` is invoked. If BLE is active, `RadioKitBLE.updateAdvertisingName(RadioKit.config.name)` is called so nearby scanners see the new name immediately.

## Risks / Trade-offs

- **[Risk] Long strings exceeding buffer sizes** → Strings are truncated safely via `strlcpy(..., sizeof(...))` to 32 bytes for `name` and 128 bytes for `description`.
- **[Risk] BLE advertising packet length limit** → BLE advertising packets have MTU/payload limits (~31 bytes legacy adv packet). RadioKit handles truncation or scan response splitting internally; 32 chars is standard.
