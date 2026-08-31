# Locomotive Bell Sound

## Purpose
Defines locomotive warning bell audio synthesis, configuration defaults, fallback parsing, and runtime volume synchronization.

## Requirements

### Requirement: Locomotive bell volume configuration and fallback
Locomotive vehicle configurations SHALL specify an audible `"bell"` volume in `sound_volumes`. If omitted, `ConfigParser` SHALL fallback to a default volume of `100` when `cfg.type == VEHICLE_LOCOMOTIVE`.

#### Scenario: Parsing locomotive vehicle config without explicit bell volume
- **WHEN** a vehicle config with `"type": "locomotive"` is loaded and `"bell"` is not in `sound_volumes`
- **THEN** `cfg.sound.bell` is set to `100` instead of `0`

#### Scenario: Parsing vehicle config with explicit bell volume
- **WHEN** a vehicle config declares `"bell": 120` in `sound_volumes`
- **THEN** `cfg.sound.bell` is set to `120`

### Requirement: Locomotive bell sound playback
When `bell_button` is activated on a locomotive, `VehicleController` SHALL trigger `BELL` voice playback in `RcEngineSound` with the configured non-zero volume.

#### Scenario: Bell button pressed
- **WHEN** `bell_button` is pressed on RadioKit Page 1 (Locomotive)
- **THEN** `RcEngineSound` plays `bell.pcm` through the I2S audio mixer with audible non-zero amplitude
