## ADDED Requirements

### Requirement: Device metadata schema support
The hardware config schema and firmware parser SHALL support optional top-level `name` (string, max 32 characters) and `description` (string, max 128 characters) properties without emitting unknown key warnings. The vehicle config parser SHALL support `vehicle.description` (string, max 128 characters) alongside `vehicle.name`.

#### Scenario: Hardware config with top-level name and description validates
- **WHEN** a hardware config includes top-level `"name": "TrackLink V3"` and `"description": "Locomotive controller"`
- **THEN** host-side schema validation passes and firmware loads both fields without unknown key warnings

#### Scenario: Vehicle config with description loads cleanly
- **WHEN** a vehicle config includes `"description": "Heavy Haul Semi Truck"` under the `"vehicle"` object
- **THEN** the firmware parser stores the description in `RcEngineSound::Config` without unknown key warnings
