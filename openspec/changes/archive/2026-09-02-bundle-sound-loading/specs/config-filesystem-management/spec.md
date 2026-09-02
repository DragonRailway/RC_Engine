## ADDED Requirements

### Requirement: Vehicle bundle sound asset resolution
The firmware SHALL resolve PCM sound asset files for each configured sound slot by prioritizing the vehicle bundle's directory hierarchy:
1. `/vehicle_configs/<sound_set>/sounds/<slot>.pcm`
2. `/vehicle_config/<sound_set>/sounds/<slot>.pcm`
3. `/sounds/vehicles/<sound_set>/<slot>.pcm`
4. `/vehicle_configs/common/<type>/<slot>.pcm`
5. `/sounds/common/<type>/<slot>.pcm`
6. `/sounds/presets/<type>/<slot>.pcm`

#### Scenario: Resolve sound from vehicle bundle directory
- **WHEN** LittleFS contains `/vehicle_configs/UnionPacific2002/sounds/bell.pcm` and the vehicle config specifies `sound_set: "UnionPacific2002"`
- **THEN** `ConfigParser::loadSounds()` resolves and loads `bell.pcm` from `/vehicle_configs/UnionPacific2002/sounds/bell.pcm`

#### Scenario: Fallback to common vehicle-type preset sound
- **WHEN** a sound slot is not present in the vehicle bundle directory but exists at `/vehicle_configs/common/locomotive/bell.pcm` or `/sounds/common/locomotive/bell.pcm`
- **THEN** `ConfigParser::loadSounds()` loads the sound asset from the common preset directory
