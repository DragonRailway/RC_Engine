## ADDED Requirements

### Requirement: Reverser Flip Momentum Interlock
The locomotive controller SHALL safely interlock direction reversals occurring while the train is in motion, preventing sudden mechanical shock and enforcing prototypical deceleration.

#### Scenario: Reverser changed while moving
- **WHEN** vehicle type is `LOCOMOTIVE`
- **AND** the locomotive has non-zero speed (`speed > 0`)
- **AND** the user flips `dir_switch`
- **THEN** internal throttle demand is clamped to 0
- **AND** the locomotive applies dynamic/friction braking until speed reaches 0 km/h
- **AND** opposing drive motor polarity is engaged only after the locomotive is stationary

#### Scenario: Throttle zeroing on direction change
- **WHEN** the reverser is flipped
- **THEN** throttle output remains zeroed until the vehicle is stationary and the user reapplies throttle
