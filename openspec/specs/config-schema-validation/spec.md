# config-schema-validation Specification

## Purpose
TBD - created by archiving change config-schema-validation. Update Purpose after archive.
## Requirements
### Requirement: Firmware warns on semantic config errors
The firmware SHALL detect and log semantic errors in hardware and vehicle configs — unrecognized `hardware` tokens, unknown keys, out-of-range values, and unrecognized enum strings — as `WARN:` lines on boot and on config hot-reload, without halting execution, and SHALL preserve the previously loaded config's behavior (degraded-but-running) exactly as before validation existed.

#### Scenario: Typo'd hardware token is reported
- **WHEN** a hardware config references an unrecognized `hardware` token (e.g. `HBRIDG_A`)
- **THEN** the firmware logs a warning naming the token and the affected output, and the output remains unconfigured (driven never)

#### Scenario: Unknown key is reported
- **WHEN** a config contains a key the parser does not read (e.g. `head_lightt`)
- **THEN** the firmware logs a warning naming the key, without failing to load the rest of the config

#### Scenario: Out-of-range value is reported
- **WHEN** a numeric parameter violates its documented range (e.g. `duty.max` above 100 or `duty.min` above `duty.max`)
- **THEN** the firmware logs a warning naming the parameter and its value

#### Scenario: Validation does not halt boot or reload
- **WHEN** any semantic warning is emitted
- **THEN** the config still loads and the system continues to boot or reload, matching pre-validation behavior

### Requirement: Accepted-but-ignored keys do not warn
The validation SHALL treat keys that the parser intentionally accepts but does not read (e.g. `lights.turn_light.type`) as valid, so no false-positive warnings are emitted for documented quirks.

#### Scenario: Documented quirk keys pass silently
- **WHEN** a config includes `"type": "blink"` under `lights.turn_light`
- **THEN** no warning is emitted for that key, and behavior is unchanged (blink driven by `interval_on`/`interval_off`)

### Requirement: Host-side schema validation at flash time
The repository SHALL provide a JSON Schema for the hardware config and SHALL validate it in `scripts/build_fs.py` before staging a filesystem image, so flash-time config mistakes are caught deterministically. A validation script SHALL also validate all configs in the repository for CI use.

#### Scenario: Invalid config rejected at flash time
- **WHEN** `build_fs.py --board <B> --vehicle <V>` runs with a hardware config that violates the schema (unknown property, out-of-range value, invalid enum)
- **THEN** the tool reports the schema violations and exits nonzero without staging or flashing

#### Scenario: All bundles validate in CI
- **WHEN** a validation script is run across the repository
- **THEN** every hardware config and vehicle bundle either passes schema validation or is reported with specific violations

### Requirement: Validation matches parser reality
The schema and firmware checks SHALL match the parser's actual behavior, including documented quirks: motor type (`hbridge`/`esc`) is derived from the `hardware` token — `drive_motor`, `left_motor`, and `right_motor` accept no `type` key — `brake_light`/`reversing_light` brightness is forced to 100 by the firmware, and `lights.reversing_light.hardware` accepts either a pin token or a light alias.

#### Scenario: Quirk-aware validation
- **WHEN** the schema is authored
- **THEN** it allows the accepted-but-ignored keys and light-alias values the parser honors, and rejects only what the parser would not honor

### Requirement: Device metadata schema support
The hardware config schema and firmware parser SHALL support optional top-level `name` (string, max 32 characters) and `description` (string, max 128 characters) properties without emitting unknown key warnings. The vehicle config parser SHALL support `vehicle.description` (string, max 128 characters) alongside `vehicle.name`.

#### Scenario: Hardware config with top-level name and description validates
- **WHEN** a hardware config includes top-level `"name": "TrackLink V3"` and `"description": "Locomotive controller"`
- **THEN** host-side schema validation passes and firmware loads both fields without unknown key warnings

#### Scenario: Vehicle config with description loads cleanly
- **WHEN** a vehicle config includes `"description": "Heavy Haul Semi Truck"` under the `"vehicle"` object
- **THEN** the firmware parser stores the description in `RcEngineSound::Config` without unknown key warnings

