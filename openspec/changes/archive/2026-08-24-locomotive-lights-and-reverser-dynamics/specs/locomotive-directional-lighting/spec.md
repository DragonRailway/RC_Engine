## ADDED Requirements

### Requirement: Directional Headlight and Marker Handover
The locomotive controller SHALL automatically couple the activation of forward headlights and rear marker/tail lights to the position of the reverser (`dir_switch`) when Headlight (Bit 0) is active in `loco_light`.

#### Scenario: Locomotive driving forward
- **WHEN** vehicle type is `LOCOMOTIVE`
- **AND** `loco_light` Bit 0 (Headlight) is set
- **AND** `dir_switch` is in Forward position (`1`)
- **THEN** `head_light` (`L1`) is driven at full configured forward brightness
- **AND** `tail_light` (`L2`) is extinguished or held at dim marker level

#### Scenario: Locomotive driving in reverse
- **WHEN** vehicle type is `LOCOMOTIVE`
- **AND** `loco_light` Bit 0 (Headlight) is set
- **AND** `dir_switch` is in Reverse position (`0`)
- **THEN** `tail_light` (`L2`) is driven at full configured reverse marker brightness
- **AND** `head_light` (`L1`) is extinguished or dimmed
