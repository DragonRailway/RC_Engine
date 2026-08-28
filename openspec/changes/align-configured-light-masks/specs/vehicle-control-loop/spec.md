## MODIFIED Requirements

### Requirement: Light control
The firmware SHALL map the `truck_light` (Truck page) and `loco_light` (Loco page) multi-select bitmasks, as well as automatic steering indicators, dynamic deceleration brake lights, and low-battery hazard overrides, to the configured light outputs (head, tail, brake, turn, reversing). The firmware SHALL compute an 8-bit configured light mask (`HardwareInit::getConfiguredLightMask`) that enables visibility exclusively for channels configured in the hardware config:
- For Trucks: Bit 0 = Head Light, Bit 1 = High Beam (enabled if `full_beam` or `head_light` configured), Bit 2 = Fog Lamp, Bit 3 = Turn / Hazard, Bit 4 = Beacon, Bit 5 = Cab Light, Bit 6 = Work Light, Bit 7 = Aux Light.
- For Locomotives: Bit 0 = Directional Headlight / Tail, Bit 1 = Fog Lamp, Bit 2 = Ditch Lights, Bit 3 = Beacon, Bit 4 = Cab Light, Bit 5 = Step Light, Bit 6 = Aux / Work Light, Bit 7 = Hazard / Warning.

#### Scenario: LED group selected
- **WHEN** an item is selected on `truck_light` or `loco_light`
- **THEN** the corresponding configured light outputs are enabled via PWM

#### Scenario: All lights deselected
- **WHEN** no items are selected on a light widget and no automation is active
- **THEN** the corresponding light outputs are disabled

#### Scenario: Configured light mask visibility filtering
- **WHEN** the board initializes or hot-reloads hardware configuration
- **THEN** `truck_light` and `loco_light` widgets are updated with item masks enabling only the buttons corresponding to configured hardware channels
