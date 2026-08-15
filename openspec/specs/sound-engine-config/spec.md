# Sound Engine Config Specification

## Purpose
Defines requirements for sound engine vehicle profiles, sound set properties, and preset inheritance.

## Requirements


### Requirement: Sound set and preset properties
The `vehicle` section of `/vehicle-config.json` SHALL support optional `"sound_set"` and `"preset"` string fields to specify the sound asset directory and inherited sound preset category.

#### Scenario: Sound set specified in vehicle config
- **WHEN** `/vehicle-config.json` specifies `"sound_set": "ScaniaV8"` and `"preset": "heavy_truck"`
- **THEN** the sound loader resolves PCM files using the 3-tier inheritance cascade

### Requirement: 3-tier hierarchical sound asset resolution
The sound loader SHALL resolve PCM sample files for each sound slot using a 3-tier lookup order: `/sounds/vehicles/<sound_set>/<slot>.json` -> `/sounds/presets/<preset>/<slot>.json` -> `/sounds/generic/<slot>.json`.

#### Scenario: Vehicle-specific sound found
- **WHEN** a sound sample file exists at `/sounds/vehicles/<sound_set>/<slot>.json`
- **THEN** the sound loader loads the PCM samples from that file

#### Scenario: Preset fallback sound loaded
- **WHEN** a sound sample file does not exist in the vehicle folder but exists at `/sounds/presets/<preset>/<slot>.json`
- **THEN** the sound loader loads the PCM samples from the preset folder

#### Scenario: Generic fallback sound loaded
- **WHEN** a sound sample file exists in neither the vehicle nor preset folders, but exists at `/sounds/generic/<slot>.json`
- **THEN** the sound loader loads the PCM samples from the generic folder
