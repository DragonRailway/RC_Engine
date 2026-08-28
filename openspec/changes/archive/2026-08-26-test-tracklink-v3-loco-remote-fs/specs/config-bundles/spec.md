## MODIFIED Requirements

### Requirement: Vehicle bundle layout
The repository SHALL organize device configuration as self-contained vehicle bundles under `configs/vehicle_configs/<sound_set>/`, where each bundle contains a `vehicle.json` (the vehicle config) and a `sounds/` subdirectory with its `<slot>.json` sound files, and the bundle directory name SHALL equal the `sound_set` declared in that bundle's `vehicle.json`. Board hardware configs SHALL live under `configs/hardware_configs/hardware-<BOARD>.json`, and shared preset/fallback sounds SHALL live under `configs/vehicle_configs/common/<preset>/`.

#### Scenario: Bundle matches its sound_set
- **WHEN** a bundle exists at `configs/vehicle_configs/<X>/`
- **THEN** its `vehicle.json` declares `sound_set: <X>` and every `<slot>.json` under its `sounds/` subdirectory is a valid sound JSON

#### Scenario: Sound-only bundles allowed
- **WHEN** a bundle contains only a `sounds/` subdirectory and no `vehicle.json`
- **THEN** it is a valid library entry that stages as `/sounds/vehicles/<set>/` and can be paired with any vehicle config declaring that `sound_set`

#### Scenario: Locomotive vehicle bundle validation
- **WHEN** the `UnionPacific2002` bundle is assembled
- **THEN** its `vehicle.json` contains `type: "locomotive"`, `sound_set: "UnionPacific2002"`, direct transmission configuration, and valid EMD sound volume definitions

#### Scenario: Hardware configs separated by board
- **WHEN** a board hardware config is added
- **THEN** it lives at `configs/hardware_configs/hardware-<BOARD>.json` with the board id matching the firmware path `/hardware-<BOARD>.json`

#### Scenario: Shared presets under common
- **WHEN** a shared preset/fallback sound exists
- **THEN** it lives at `configs/vehicle_configs/common/<preset>/<slot>.json`, mapping to the firmware's `/sounds/presets/<preset>/<slot>.json` fallback tier
