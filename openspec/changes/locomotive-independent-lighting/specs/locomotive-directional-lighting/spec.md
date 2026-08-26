# Locomotive Directional Lighting

## MODIFIED Requirements

### Requirement: Directional Headlight and Marker Handover
The locomotive controller SHALL couple the activation of forward headlights and rear marker/tail lights to the position of the reverser (`dir_switch`) when Headlight (Bit 0) is active in `loco_light`, without any cross-button interlocks to other lighting channels.

#### Scenario: Locomotive driving forward
- **WHEN** vehicle type is `LOCOMOTIVE`
- **AND** `loco_light` Bit 0 (Headlight) is set
- **AND** `dir_switch` is in Forward position (`1`)
- **THEN** `head_light` (`L1`) is driven at full configured forward brightness
- **AND** `tail_light` (`L2`) is extinguished (`0`)

#### Scenario: Locomotive driving in reverse
- **WHEN** vehicle type is `LOCOMOTIVE`
- **AND** `loco_light` Bit 0 (Headlight) is set
- **AND** `dir_switch` is in Reverse position (`0`)
- **THEN** `tail_light` (`L2`) is driven at full configured reverse brightness
- **AND** `head_light` (`L1`) is extinguished (`0`)

#### Scenario: Master headlight off
- **WHEN** vehicle type is `LOCOMOTIVE`
- **AND** `loco_light` Bit 0 (Headlight) is cleared (`0`)
- **THEN** both `head_light` and `tail_light` are extinguished (`0`)
- **AND** other active bits in `loco_light` (e.g. Bit 1, Bit 2) remain unchanged
