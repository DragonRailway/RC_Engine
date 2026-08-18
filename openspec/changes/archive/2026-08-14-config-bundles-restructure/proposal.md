## Why

`data/` is a 132 MB grab-bag (126 MB of sounds for 76 vehicles + configs + test artifacts) that can never fit the board's 896 KB LittleFS partition, so `pio run -t uploadfs` always fails with `LFS_ERR_NOSPC` and every FS bring-up requires manual staging. At the same time, a vehicle config and the sounds it references are coupled (the config's `sound_set` names the sound directory) but live far apart, and three top-level vehicle config files are near-duplicates. Restructure the repo around a *bundle model* — one folder per vehicle containing its config and its sounds, hardware configs beside them, shared preset sounds in one common place — with a manifest-driven FS builder that stages exactly one board + one vehicle onto the device.

## What Changes

- **New `configs/` layout** (the user-facing source of truth):
  - `configs/hardware_configs/hardware-<BOARD>.json` — the two board hardware configs (filenames keep the `<BOARD>` id matching the firmware path `/hardware-<BOARD>.json`)
  - `configs/vehicle_configs/<sound_set>/` — **76 vehicle bundles**, each containing `vehicle.json` (the vehicle config) + a `sounds/` subdirectory with its `<slot>.json` sound files. Bundle dir name = `sound_set` id, which is what the firmware sound loader already uses
  - `configs/vehicle_configs/common/` — shared preset/fallback sounds (heavy_truck, excavator, locomotive), replacing the old `presets/` tree
- **`data/` demoted** to gitignored transient scratch (staging/build outputs only; nothing tracked lives there anymore)
- **`scripts/build_fs.py`** — manifest-driven FS deployment: `--board MIKRO_V2 --vehicle ScaniaV8` assembles the selected hardware config + vehicle bundle (+ common fallbacks) into a LittleFS image and flashes it at the spiffs partition; errors if the assembled set exceeds the partition size
- **BREAKING (tooling):** `pio run -t uploadfs` no longer produces a valid filesystem from the repo (configs/sounds moved out of `data/`). `build_fs.py` becomes the only FS-flash path — a guardrail (fail-loud) prevents flashing a config-less FS
- **Script path updates:** `validate_sounds.py`, `hot_reload_test.py`, the sound conversion/organization tools, and the DSP verification scripts (`host_dsp_test.py`, `compare_reference_engine.py`, `golden_metrics.py`, `verify_reference_parity.py`) point at the new locations
- **`golden_audio_profile.json`** moves to `test/golden/` and becomes tracked (it is the DSP regression baseline)
- **Not breaking:** firmware runtime paths are unchanged — the board still reads `/hardware-<BOARD>.json`, `/vehicle-config.json`, `/sounds/vehicles/<set>/<slot>.json`; RadioKit config upload + hot-reload keeps working; no firmware code changes

## Capabilities

### New Capabilities
- `config-bundles`: The repo-level bundle layout (configs/hardware_configs, configs/vehicle_configs/<sound_set> bundles with vehicle.json + sounds, configs/vehicle_configs/common shared presets) and the manifest-driven FS deployment tooling (one hardware config + one vehicle bundle per board, partition-size guard) that stages them onto the device.

### Modified Capabilities
<!-- none — firmware runtime behavior (config-filesystem-management, etc.) is unchanged; this is repo/tooling only -->

## Impact

- **Repo layout**: `data/sounds/vehicles/*` (~76 dirs, ~1,300 files) → `configs/vehicle_configs/*`; `data/*.json` configs → `configs/hardware_configs/` + bundles; large but mechanical git rename (content unchanged)
- **`data/`**: becomes gitignored scratch (add to `.gitignore`)
- **Scripts**: `scripts/validate_sounds.py`, `scripts/hot_reload_test.py`, `scripts/organize_sounds.py`, `scripts/organize_by_raw_vehicles.py`, `scripts/convert_raw_sounds_to_json.py`, `scripts/convert_sounds.sh`, `scripts/host_dsp_test.py`, `scripts/compare_reference_engine.py`, `scripts/golden_metrics.py`, `scripts/verify_reference_parity.py`
- **Docs**: `AGENTS.md`, `docs/audio_debug_tooling.md`, OpenSpec specs referencing `data/sounds/` (`audio-debug-tooling`)
- **platformio.ini**: unchanged for firmware envs; `uploadfs` becomes a footgun (guardrail added)
- **Firmware**: none
