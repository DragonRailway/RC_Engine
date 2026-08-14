# config-bundles Specification

## Purpose

Define how device configuration is organized as self-contained, deployable bundles in the repository, and how those bundles are deployed to a board's LittleFS partition. The board can only hold one hardware config and one vehicle bundle, so the repo acts as a library of deployable combinations.

## Requirements

### Requirement: Vehicle bundle layout
The repository SHALL organize device configuration as self-contained vehicle bundles under `configs/vehicle_configs/<sound_set>/`, where each bundle contains a `vehicle.json` (the vehicle config) and a `sounds/` subdirectory with its `<slot>.json` sound files, and the bundle directory name SHALL equal the `sound_set` declared in that bundle's `vehicle.json`. Board hardware configs SHALL live under `configs/hardware_configs/hardware-<BOARD>.json`, and shared preset/fallback sounds SHALL live under `configs/vehicle_configs/common/<preset>/`.

#### Scenario: Bundle matches its sound_set
- **WHEN** a bundle exists at `configs/vehicle_configs/<X>/`
- **THEN** its `vehicle.json` declares `sound_set: <X>` and every `<slot>.json` under its `sounds/` subdirectory is a valid sound JSON

#### Scenario: Sound-only bundles allowed
- **WHEN** a bundle contains only a `sounds/` subdirectory and no `vehicle.json`
- **THEN** it is a valid library entry that stages as `/sounds/vehicles/<set>/` and can be paired with any vehicle config declaring that `sound_set`

#### Scenario: Hardware configs separated by board
- **WHEN** a board hardware config is added
- **THEN** it lives at `configs/hardware_configs/hardware-<BOARD>.json` with the board id matching the firmware path `/hardware-<BOARD>.json`

#### Scenario: Shared presets under common
- **WHEN** a shared preset/fallback sound exists
- **THEN** it lives at `configs/vehicle_configs/common/<preset>/<slot>.json`, mapping to the firmware's `/sounds/presets/<preset>/<slot>.json` fallback tier

### Requirement: Manifest-driven filesystem deployment
The repository SHALL provide `scripts/build_fs.py` that stages exactly one board hardware config and one vehicle bundle (plus the shared common fallbacks) into a LittleFS image for the selected board, honoring the partition size, and flashes it at the spiffs partition derived from the partition table. The tool SHALL exit with an error when the assembled image exceeds the partition size or when required configuration files are missing, and SHALL be the only supported filesystem-flash path.

#### Scenario: Deploy a board and vehicle
- **WHEN** `build_fs.py --board MIKRO_V2 --vehicle ScaniaV8` runs successfully
- **THEN** the flashed filesystem contains `/hardware-MIKRO_V2.json`, `/vehicle-config.json` (from the bundle's `vehicle.json`), `/sounds/vehicles/ScaniaV8/*`, and the common preset fallbacks

#### Scenario: Partition overflow guarded
- **WHEN** the assembled set of files exceeds the LittleFS partition size
- **THEN** `build_fs.py` reports the overflow and exits nonzero without flashing

#### Scenario: Missing config fails fast
- **WHEN** the selected board hardware config or vehicle bundle does not exist
- **THEN** `build_fs.py` reports the missing files and exits nonzero without flashing

#### Scenario: PlatformIO uploadfs blocked
- **WHEN** `pio run -t uploadfs` (or `buildfs`) is invoked after the bundle restructure
- **THEN** a guardrail script fails the build with a message pointing to `build_fs.py`, because `data/` no longer contains the staged configs

### Requirement: data/ as transient scratch
The `data/` directory SHALL be treated as transient, gitignored scratch: no tracked configuration files, sound assets, or regression baselines SHALL live under `data/`. Tracked regression baselines (e.g. the audio golden profile) SHALL live under `test/`.

#### Scenario: No tracked configs in data/
- **WHEN** the repository is inspected
- **THEN** no tracked files under `data/` exist (it holds only gitignored build/test outputs)

#### Scenario: Baselines tracked under test/
- **WHEN** a regression baseline is committed
- **THEN** it lives under `test/` (e.g. `test/golden/golden_audio_profile.json`) and is tracked by git
