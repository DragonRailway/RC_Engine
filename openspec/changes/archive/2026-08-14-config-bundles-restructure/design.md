## Context

The board's LittleFS partition is 896 KB (`spiffs` @ 0x310000, size 0xE0000, from `partitions_ota_4MB.csv`). The repo's `data/` tree is 132 MB (126 MB of sound JSONs for 76 vehicles + configs + test artifacts) — so PlatformIO's `uploadfs`, which builds the FS image from the hardcoded `data/` directory, always fails with `LFS_ERR_NOSPC`. Every FS bring-up today requires manual staging (build a trimmed `data/`, flash, restore). Meanwhile the coupling between a vehicle config and its sounds is implicit: `vehicle-config.json` declares `sound_set: "ScaniaV8"`, and the firmware resolves `/sounds/vehicles/ScaniaV8/<slot>.json` at runtime. The three top-level vehicle config files are near-duplicates.

The board can only ever hold **one hardware config and one vehicle config** (single `/hardware-<BOARD>.json`, single `/vehicle-config.json`). So the repo should be a *library* of deployable bundles: `configs/vehicle_configs/<sound_set>/` = one vehicle (config + its sounds), `configs/hardware_configs/` = the board configs, `configs/vehicle_configs/common/` = shared preset fallbacks.

## Goals / Non-Goals

**Goals:**
- One self-contained folder per vehicle (config + sounds), dir name = `sound_set` id
- Hardware configs in their own `configs/hardware_configs/` dir
- Shared preset/fallback sounds under `configs/vehicle_configs/common/`
- A manifest-driven FS builder (`build_fs.py --board <B> --vehicle <V>`) as the only FS-flash path, with a partition-size guard
- `data/` demoted to gitignored scratch; tracked baselines (`golden_audio_profile.json`) moved to `test/`
- Zero firmware changes — runtime LittleFS paths stay exactly as today

**Non-Goals:**
- No changes to the firmware's sound-resolution logic, config schema, or runtime paths
- No rename mapping for bundles (dir name must equal `sound_set`)
- No multi-vehicle/multi-config on a single board
- No PlatformIO builder patching (the platform's `data/`-hardcoded builder stays abandoned)

## Decisions

### D1: Bundle directory name = `sound_set` id
`configs/vehicle_configs/ScaniaV8/`, `configs/vehicle_configs/Caterpillar323/`, etc. — the directory name *is* the `sound_set` the vehicle config declares, which is exactly the directory the firmware loader resolves at `/sounds/vehicles/<sound_set>/`. No rename mapping, no config schema change.
- *Alternative considered:* friendly display names (`configs/vehicle_configs/my-scania-truck/`). Rejected — would need a rename table and risks breaking sound resolution.

### D2: Nested bundle layout with `sounds/` subdirectory
Each bundle is self-documenting: `vehicle.json` (the config) at the top, all `<slot>.json` sound files under a `sounds/` subdirectory. The staging rule is explicit and unambiguous — `vehicle.json` → `/vehicle-config.json`, `sounds/*.json` → `/sounds/vehicles/<sound_set>/` — and a user browsing the bundle immediately sees the manifest separate from the assets. This mirrors the on-device `/sounds/` namespace, so the mapping reads naturally.
- *Alternative considered:* flat bundle (config + slots side by side). Rejected by decision — the manifest/assets split is clearer, and the loader's `/sounds/` namespace maps 1:1 onto the `sounds/` subdir.

### D3: `common/` keeps per-preset subdirectories
`configs/vehicle_configs/common/<preset>/<slot>.json` (heavy_truck, excavator, locomotive) maps 1:1 to the firmware's `/sounds/presets/<preset>/<slot>.json` fallback tier. The loader's 3-tier resolution (vehicles/<set> → presets/<preset> → generic) is preserved by staging; the empty `generic/` tier is dropped.
- *Alternative considered:* flat `common/` with slot files. Rejected — loses the preset keying the loader depends on.

### D4: `build_fs.py` builds the FS image directly (no `data/` mutation)
The builder assembles `configs/hardware_configs/hardware-<BOARD>.json` + `configs/vehicle_configs/<V>/vehicle.json` + `configs/vehicle_configs/<V>/sounds/*` + `configs/vehicle_configs/common/` into a staging dir, runs the littlefs-python builder (the same package the pioarduino platform uses for `uploadfs`; the espressif32 platform ships no `mklittlefs` binary) to produce `littlefs.bin`, and flashes it with `esptool` at the spiffs partition offset parsed from `partitions_ota_4MB.csv`. It fails loudly if the assembled size exceeds the partition. No symlink swaps, no temporary `data/` mutation.
- *Alternative considered:* keep PlatformIO `uploadfs` with a transient `data/` swap (the manual procedure that worked during bring-up). Rejected — mutating `data/` is fragile and the `data_dir` option doesn't exist in this platform version (verified).
- *Alternative considered:* RadioKit OTA upload for every change. Still supported and preferred for config-only tweaks, but OTA can't stage a whole sound set in one shot (per-file), so `build_fs.py` covers first bring-up and sound-set swaps.

### D5: `data/` becomes gitignored scratch
Nothing tracked lives in `data/` anymore. It remains as a place for transient build/test outputs (generated `.wav`/`.raw` captures, staging leftovers) — which also neutralizes the untracked-artifact-loss class of problem (scratch is expected to be disposable). `golden_audio_profile.json`, the tracked DSP regression baseline, moves to `test/golden/`.

### D6: Guardrail against empty-FS flashes
A naked `pio run -t uploadfs` after the move would build an FS from scratch/empty `data/` → boot FATAL (`Cannot open: /hardware-<BOARD>.json`). Mitigation: `build_fs.py` is documented as the only FS path, and a PlatformIO `pre:uploadfs` check script fails fast when `data/` does not contain the staged configs (i.e., nobody should be running uploadfs at all).
- *Alternative considered:* delete the `uploadfs` target. Not possible cleanly from `platformio.ini`; the pre-script + docs are sufficient.

## Risks / Trade-offs

- **Large git rename** (~76 dirs / ~1,300 files) against an already-dirty working tree (pre-session door.json edits) → Stage the moves as content-free renames (`git mv`/`mv` + `git add -A`) so git records them as renames; keep content edits separate.
- **Sound resolution silently breaks if a bundle dir is renamed** → `validate_sounds.py` gains a layout check: bundle dir name must match the `sound_set` in its `vehicle.json`; also validates every slot file under `sounds/` is a valid sound JSON.
- **Empty-FS boot FATAL from stray `uploadfs`** → D6 guardrail (pre-script + docs).
- **FS tool / `esptool` availability** → `build_fs.py` uses the `littlefs` Python package from the PlatformIO penv (the same one the pioarduino platform uses for `uploadfs`; the espressif32 platform ships no `mklittlefs` binary) and `esptool.py` from `packages/tool-esptoolpy`, with a clear error if missing. The board chip is resolved from the env's `board` in `platformio.ini`.
- **DSP verification scripts lose their `data/` outputs home** → they already write generated `.wav`/`.raw`; these remain scratch under gitignored `data/` (or `.pio/`), and only the tracked baseline moves to `test/golden/`.
- **Rollback** → the restructure is a reorg, not a behavior change; `git revert` of the move restores `data/` as the FS source (runtime unaffected in either direction).

## Migration Plan

1. Create `configs/hardware_configs/`, `configs/vehicle_configs/`; `git mv` the 5 top-level config JSONs and all 76 sound dirs, placing each vehicle's sounds into `configs/vehicle_configs/<set>/sounds/` (`data/sounds/vehicles/<set>/*` → `configs/vehicle_configs/<set>/sounds/*`; `data/sounds/presets/*` → `configs/vehicle_configs/common/*`); collapse the 3 vehicle config files into their bundles (`vehicle-config.json` ≈ `vehicle-ScaniaV8.json` → `ScaniaV8/vehicle.json`, `vehicle-Caterpillar323.json` → `Caterpillar323/vehicle.json`)
2. Move `golden_audio_profile.json` → `test/golden/`; add `data/` to `.gitignore`
3. Write `scripts/build_fs.py` (assemble → mklittlefs → esptool, partition parse, size guard) and validate it end-to-end by flashing MIKRO_V2
4. Update scripts: `validate_sounds.py` (+ bundle-layout checks), `hot_reload_test.py`, conversion/organization tools, DSP verification scripts
5. Add the `uploadfs` guardrail pre-script; update `AGENTS.md`, `docs/audio_debug_tooling.md`
6. Verify: clean clone boots after `build_fs.py --board MIKRO_V2 --vehicle ScaniaV8`; config-only change still hot-reloads via RadioKit

## Open Questions

- Should `build_fs.py` also offer a `--dry-run` size report? (Nice-to-have; default yes, trivial.)
- Keep the flat legacy sound paths (`/sounds/<soundSet>-<slot>.json`) referenced by the loader? They're fallbacks for assets that no longer exist in the repo — drop from staging (loader already skips missing files).
