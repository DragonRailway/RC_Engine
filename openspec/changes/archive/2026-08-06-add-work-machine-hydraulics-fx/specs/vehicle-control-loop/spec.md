## MODIFIED Requirements

### Requirement: Horn and sound engine drive
The firmware SHALL trigger the horn sound while `horn` is pressed and SHALL feed throttle and hydraulic governor pump load into the sound engine so engine RPM and pitch follow gas pedal, Loco slider, and active hydraulic flow inputs.

#### Scenario: Horn pressed
- **WHEN** `horn` is pressed
- **THEN** the sound engine plays the horn sound at its configured volume

#### Scenario: Throttle raises engine RPM
- **WHEN** the gas pedal, Loco slider, or hydraulic flow demand increases
- **THEN** the sound engine's simulated RPM rises and sound pitch follows
