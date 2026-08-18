## ADDED Requirements

### Requirement: Document Full Beam and Fog Lamp in Hardware Guide
The hardware reference guide at `GUIDE/HARDWARE_CONFIG.md` SHALL document `lights.full_beam` and `lights.fog_lamp` with their parameter names, types, defaults, and physical wiring roles.

#### Scenario: Looking up full beam configuration
- **WHEN** a user checks the reference guide for `lights.full_beam`
- **THEN** they learn how to assign a dedicated high beam pin (`hardware`), its `brightness_max`, and its activation on High Beam (Bit 1)

#### Scenario: Looking up fog lamp configuration
- **WHEN** a user checks the reference guide for `lights.fog_lamp`
- **THEN** they learn how to assign the dedicated fog lamp pin (`hardware`), its independence from locomotive ditch lights, and its activation on Fog Lamp (Bit 2)
