## ADDED Requirements

### Requirement: 8-Bit Multiple-Select Light Grid Dispatching
The system SHALL map the 8-bit MultipleSelect control widget mask to distinct lighting outputs across vehicle types:
- Bit 0 (`0x01`): Head Light (`lights.head_light`)
- Bit 1 (`0x02`): Full Beam (`lights.full_beam`)
- Bit 2 (`0x04`): Fog Lamp (`lights.fog_lamp`)
- Bit 3 (`0x08`): Hazard Lights (Truck) / Ditch Lights (`lights.ditch_light`, Loco)
- Bit 4 (`0x10`): Beacon Light (`lights.beacon`)
- Bit 5 (`0x20`): Cab Light (`lights.cab_light`)
- Bit 6 (`0x40`): Work Light (`lights.work_light`, Truck) / Step Light (`lights.step_light`, Loco)
- Bit 7 (`0x80`): Aux Light (`lights.aux_light`)

#### Scenario: Activating Beacon Light via Bit 4
- **WHEN** Bit 4 (`0x10`) is set on the light control widget and `lights.beacon` is configured
- **THEN** the beacon output pin is energized and executes its configured strobe/pattern

#### Scenario: Activating Work Light via Bit 6 on Truck
- **WHEN** Bit 6 (`0x40`) is set on the truck light widget and `lights.work_light` is configured
- **THEN** the work light output pin is energized to its configured brightness

#### Scenario: Unconfigured Buttons Disabled
- **WHEN** the firmware starts and determines which lighting channels are configured
- **THEN** unconfigured lighting channels have their corresponding buttons disabled in the UI
