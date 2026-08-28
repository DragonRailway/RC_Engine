# Locomotive Lighting Effects

## MODIFIED Requirements

### Requirement: Ditch Light Triangular Cross-Fade
The locomotive controller SHALL alternate dual ditch lights (`left` / `L4` and `right` / `L5`) using a continuous triangular/sinusoidal cross-fade curve purely when Ditch Lights are manually activated via `loco_light` Bit 2 (`0x04`).

#### Scenario: Ditch lights manually activated
- **WHEN** `loco_light` Bit 2 is set (`0x04`)
- **THEN** left fixture brightness ramps up while right fixture brightness ramps down inversely
- **AND** the cycle reverses smoothly when reaching duty boundaries

#### Scenario: Ditch lights deactivated
- **WHEN** `loco_light` Bit 2 is cleared (`0`)
- **THEN** both ditch light fixtures are immediately extinguished (`0`)
- **AND** bell or horn activations do NOT automatically trigger ditch lights
