## 1. Restructure Repo Layout

- [x] 1.1 Create `configs/hardware_configs/` and `configs/vehicle_configs/`; move the top-level config JSONs: `hardware-MIKRO_V2.json`/`hardware-TRACKLINK_V3.json` → `configs/hardware_configs/`, `vehicle-config.json` + `vehicle-ScaniaV8.json` → `configs/vehicle_configs/ScaniaV8/vehicle.json`, `vehicle-Caterpillar323.json` → `configs/vehicle_configs/Caterpillar323/vehicle.json`
- [x] 1.2 `git mv` all 76 sound dirs, placing each vehicle's slot files into a `sounds/` subdir: `data/sounds/vehicles/<set>/*` → `configs/vehicle_configs/<set>/sounds/*` (bundle dirs keep their `sound_set` names)
- [x] 1.3 `git mv` shared preset sounds `data/sounds/presets/*` → `configs/vehicle_configs/common/*`
- [x] 1.4 Move `data/golden_audio_profile.json` → `test/golden/golden_audio_profile.json`; add `data/` to `.gitignore`
- [x] 1.5 Remove the now-empty `data/sounds/` tree and dead `generic/` tier from the repo layout

## 2. Manifest-Driven FS Deployment (build_fs.py)

- [x] 2.1 Implement `scripts/build_fs.py`: `--board <B> --vehicle <V>`; parse `partitions_ota_4MB.csv` for the spiffs partition (offset/size); assemble a staging dir (hardware config + `vehicle.json` + `sounds/*` + `common/` presets mapped to `/hardware-<B>.json`, `/vehicle-config.json`, `/sounds/vehicles/<V>/`, `/sounds/presets/<preset>/`); build `littlefs.bin` via littlefs-python (the same package the pioarduino platform uses for `uploadfs`; the espressif32 platform has no `mklittlefs` package); flash via `esptool` (chip resolved from the env's `board` in `platformio.ini`)
- [x] 2.2 Add fail-fast guards: partition overflow (assembled size > partition), missing hardware config / vehicle bundle, missing tools — all exit nonzero with a clear message; add `--dry-run` size report. Note: empty preset dirs are NOT staged (an empty dir costs a littlefs metadata pair and pushed ScaniaV8 over the partition limit)
- [x] 2.3 Validate end-to-end on hardware: `build_fs.py --board MIKRO_V2 --vehicle ScaniaV8` → board boots to `System Ready` with 14 sounds loaded

## 3. Script Path Updates

- [x] 3.1 Update `scripts/validate_sounds.py` to walk `configs/vehicle_configs/` and add bundle-layout checks (bundle dir name == `sound_set` in its `vehicle.json`; every slot file under `sounds/` is a valid sound JSON). Note: sound-only bundles (no vehicle.json) are valid library entries; `common/` presets are skipped as non-bundles
- [x] 3.2 Update `scripts/hot_reload_test.py` to read `configs/hardware_configs/hardware-MIKRO_V2.json`
- [x] 3.3 Update conversion/organization tools (`organize_sounds.py`, `organize_by_raw_vehicles.py`, `convert_raw_sounds_to_json.py`) for the new paths — raw sources from `references/raw_sounds`/`references/raw_vehicles`, output into `configs/vehicle_configs/<set>/sounds/` (convert_sounds.sh does not exist; task text was stale)
- [x] 3.4 Update DSP verification scripts (`compare_reference_engine.py`, `golden_metrics.py`): golden baseline → `test/golden/`; `host_dsp_test.py`/`verify_reference_parity.py` already write generated `.wav`/`.raw` scratch to gitignored `data/` and need no path change

## 4. Guardrail & Docs

- [x] 4.1 Add a PlatformIO `pre:` check script (`scripts/check_uploadfs_guard.py`, wired in `platformio.ini`) that fails fast when `data/` does not contain the staged configs — verified: `uploadfs`/`buildfs` blocked with a clear message, normal builds unaffected
- [x] 4.2 Update `AGENTS.md`, `docs/audio_debug_tooling.md`, and the `audio-debug-tooling` main spec to reference the new layout (`configs/vehicle_configs/`, `configs/hardware_configs/`)

## 5. Validation

- [x] 5.1 Both firmware envs (`pio run -e TRACKLINK_V3`, `-e MIKRO_V2`) still build; host VC test passes
- [x] 5.2 `build_fs.py` deployment boots to `System Ready` (14 ScaniaV8 sounds); a config-only change still hot-reloads live via RadioKit FS upload (verified: reload triggered OK, no MCPWM errors)
- [x] 5.3 `scripts/validate_sounds.py` passes across all bundles: 1195/1195 sound files valid, 0 bundle layout failures
