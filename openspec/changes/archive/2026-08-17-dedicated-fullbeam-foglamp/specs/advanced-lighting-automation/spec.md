## ADDED Requirements

### Requirement: Dedicated Full Beam and Fog Lamp Outputs
The system SHALL support independent physical pin configurations for `full_beam` (high beam) and `fog_lamp` (fog lamp), completely independent from low beam (`head_light`) and locomotive ditch lights (`ditch_light`).

#### Scenario: Full beam activated with dedicated pin
- **WHEN** High Beam (Bit 1) is selected on the lighting control surface and `lights.full_beam` is configured
- **THEN** the dedicated `full_beam` output pin is energized to its configured brightness

#### Scenario: Fog lamp activated with dedicated pin
- **WHEN** Fog Lamp (Bit 2) is selected on the lighting control surface and `lights.fog_lamp` is configured
- **THEN** the dedicated `fog_lamp` output pin is energized to its configured brightness

#### Scenario: Full beam fallback when unconfigured
- **WHEN** High Beam (Bit 1) is selected and `lights.full_beam` is not configured
- **THEN** the system falls back gracefully to driving `head_light` at 100% duty
