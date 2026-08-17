## Context

The RadioKit RC vehicle control platform uses MultipleSelect widgets (`truck_light` and `loco_light`) to control lighting states on scale trucks, construction equipment, and locomotives. Each MultipleSelect widget supports an 8-bit bitmask where each bit maps to an interactive button.

Previously, only 3 items were actively defined in the UI design, several channels like `beacon` and `work_light` were unrepresented or conflated with locomotive ditch lights, and unconfigured hardware pins remained enabled/clickable on the UI.

This design defines the technical architecture for the 8-bit lighting grid, beacon pattern execution, work light dispatching, and dynamic unconfigured button disabling.

## Goals / Non-Goals

**Goals:**
- Unify the 8-bit MultipleSelect control widget mask to:
  - Bit 0: Head Light (`lights.head_light`)
  - Bit 1: Full Beam (`lights.full_beam`)
  - Bit 2: Fog Lamp (`lights.fog_lamp`)
  - Bit 3: Hazard Light (Truck) / Ditch Light (`lights.ditch_light`, Loco)
  - Bit 4: Beacon Light (`lights.beacon`)
  - Bit 5: Cab Light (`lights.cab_light`)
  - Bit 6: Work Light (`lights.work_light`, Truck) / Step Light (`lights.step_light`, Loco)
  - Bit 7: Aux Light (`lights.aux_light`)
- Add `Light beacon` (supporting strobe/rotary patterns) and `Light workLight` to `HardwareConfig::Lights`.
- Compute the board's configured lights bitmask in `HardwareInit`/`VehicleController` and propagate it to RadioKit UI.
- Update `docs/radiokit-rc-ui-design.json` and `src/RADIOKIT.h` to define the 8 items with appropriate labels and icons.
- Update `configs/schemas/hardware_config.schema.json` and documentation.

**Non-Goals:**
- Moving turn indicators (`turn_light.left` and `turn_light.right`) into the Multiple widget (they remain dedicated toggle buttons).
- Altering sound engine slot mapping or transmission logic.

## Decisions

### 1. Data Structures in `HardwareConfig::Lights`
In `common/Config.h`:
- `Light beacon`: Roof beacon / strobe light.
- `Light workLight`: Work / flood light.
- `Light cabLight`, `stepLight`, `auxLight`, `fogLamp`, `fullBeam`, `headLight`, `turnLight`, `ditchLight`, `brakeLight`, `reversingLight`.

### 2. VehicleController 8-Bit Dispatching
In `common/VehicleController.h`:
- Decode each bit from `uint8_t bits`:
  - `headLight  = (bits & 0x01)`
  - `fullBeam   = (bits & 0x02)`
  - `fogLamp    = (bits & 0x04)`
  - `hazardOn   = (bits & 0x08)` (Truck) / `ditchOn = (bits & 0x08)` (Loco)
  - `beaconOn   = (bits & 0x10)`
  - `cabOn      = (bits & 0x20)`
  - `workOn     = (bits & 0x40)` (Truck) / `stepOn = (bits & 0x40)` (Loco)
  - `auxOn      = (bits & 0x80)`
- Beacon strobe execution: When active, `HardwareInit::setBeacon(beaconOn)` flashes or strobes the configured pin.
- Work light execution: `HardwareInit::setLight(L.workLight.pin, workOn ? L.workLight.brightness : 0)`.

### 3. Dynamic UI Disabled Buttons
- `VehicleController::getConfiguredLightMask()` calculates:
  ```cpp
  uint8_t mask = 0;
  if (L.headLight.configured)   mask |= (1 << 0);
  if (L.fullBeam.configured || L.headLight.configured) mask |= (1 << 1);
  if (L.fogLamp.configured)    mask |= (1 << 2);
  if (L.turnLight.leftPin != 0xFF || L.ditchLight.leftPin != 0xFF) mask |= (1 << 3);
  if (L.beacon.configured)     mask |= (1 << 4);
  if (L.cabLight.configured)   mask |= (1 << 5);
  if (L.workLight.configured || L.stepLight.configured) mask |= (1 << 6);
  if (L.auxLight.configured)   mask |= (1 << 7);
  ```
- Any unconfigured bit in `mask` causes the UI button to be marked inactive/disabled or filtered out.

## Risks / Trade-offs

- **[Risk] Backward Compatibility for Older 3-Item UI Designs**: Older designs might only send 3 bits.
  - *Mitigation*: Bitmask dispatch is completely bit-independent; bits 3..7 evaluate to 0 if not present, safely keeping inactive features off.
