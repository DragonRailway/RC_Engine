## Why

When operating in Locomotive mode, pressing `bell_button` on the RadioKit UI correctly triggers the `triggerBell(true)` event in `VehicleController`, but produces no sound output. The root cause is that `UnionPacific2002/vehicle.json` omitted `"bell"` from its `sound_volumes` dictionary, causing `ConfigParser` to set `cfg.sound.bell = 0` and the sound engine to multiply bell samples by 0.

## What Changes

- **Add `"bell": 120` to `UnionPacific2002/vehicle.json`**: Set an audible warning bell volume in the locomotive configuration.
- **Smart Fallback in `ConfigParser`**: In `parseSoundVolumes()`, provide a default fallback of `100` for locomotive vehicle profiles when `"bell"` is not explicitly configured.
- **Voice Volume Synchronization in `RcEngineSound::setConfig()`**: Ensure voice volumes (including `voices[BELL].volume`) are synchronized when new configs are applied or hot-reloaded.
- **Hardware Flash & Verification**: Deploy the updated vehicle bundle to the `TRACKLINK_V3` board and verify audible bell playback when pressing `bell_button`.

## Capabilities

### New Capabilities
- `locomotive-bell-sound`: Bell volume configuration, automatic fallback for locomotive profiles, and dynamic volume sync in the sound engine.

### Modified Capabilities
*(None. Existing trigger APIs and UI widgets remain unchanged.)*

## Impact

- **`configs/vehicle_configs/UnionPacific2002/vehicle.json`**: Added `"bell": 120` to `sound_volumes`.
- **`common/ConfigParser.cpp`**: Added locomotive fallback volume for bell.
- **`lib/SoundEngine/src/RcEngineSound.h` / `RcEngineSound.cpp`**: Voice volume synchronization in `setConfig()`.
