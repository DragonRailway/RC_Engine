## Context

The RadioKit RC vehicle control platform uses 8-item MultipleSelect widgets (`truck_light` and `loco_light`) on the mobile app UI. When the firmware starts or reloads configs, `HardwareInit::getConfiguredLightMask(L, isLoco)` computes an 8-bit mask representing which light channels are configured on the board, and passes it to `setItemMask()`. The RadioKit app reads `itemMask` and hides unconfigured buttons while rendering configured ones.

Currently, `getConfiguredLightMask` for locomotives maps bits using an incorrect layout, causing configured ditch lights and cab lights to be hidden, and unconfigured snowflake/beacon/aux lights to be displayed.

## Goals / Non-Goals

**Goals:**
- Correct `HardwareInit::getConfiguredLightMask()` in `common/HardwareInit.h` to match the exact dispatch channels in `VehicleController.h` and the UI.
- Ensure only configured channels have their corresponding bits set in `itemMask`.
- Keep Truck High Beam fallback logic intact (High Beam bit is valid on truck if `full_beam` or `head_light` is configured).
- Validate behavior via host tests and live over RadioKit Remote API with the connected TrackLink V3.

**Non-Goals:**
- Changing widget wire protocol formats or resizing widgets.
- Changing pin mappings or sound engine playback.

## Decisions

### Decision 1: Canonical 8-Bit Light Mask Mapping

For Locomotives (`isLoco == true`):
- **Bit 0 (`0x01`)**: Directional Headlight / Tail (`L.headLight.configured || L.tailLight.configured`)
- **Bit 1 (`0x02`)**: Fog Lamp / Aux (`L.fogLamp.configured`)
- **Bit 2 (`0x04`)**: Ditch Lights (`L.ditchLight.configured`)
- **Bit 3 (`0x08`)**: Beacon Light (`L.beacon.configured`)
- **Bit 4 (`0x10`)**: Cab Light (`L.cabLight.configured`)
- **Bit 5 (`0x20`)**: Step Light (`L.stepLight.configured`)
- **Bit 6 (`0x40`)**: Aux / Work Light (`L.auxLight.configured || L.workLight.configured || auxPinCount > 0`)
- **Bit 7 (`0x80`)**: Hazard Warning (`L.turnLight.configured`)

For Trucks (`isLoco == false`):
- **Bit 0 (`0x01`)**: Head Light (`L.headLight.configured`)
- **Bit 1 (`0x02`)**: High Beam (`L.fullBeam.configured || L.headLight.configured`)
- **Bit 2 (`0x04`)**: Fog Lamp (`L.fogLamp.configured`)
- **Bit 3 (`0x08`)**: Turn / Hazard Light (`L.turnLight.configured`)
- **Bit 4 (`0x10`)**: Beacon Light (`L.beacon.configured`)
- **Bit 5 (`0x20`)**: Cab Light (`L.cabLight.configured`)
- **Bit 6 (`0x40`)**: Work Light (`L.workLight.configured`)
- **Bit 7 (`0x80`)**: Aux Light (`L.auxLight.configured || auxPinCount > 0`)

## Risks / Trade-offs

- [Risk] Custom hardware configs using unusual light names might have buttons hidden.
  → Mitigation: Standard schema defines all supported light tokens. Unconfigured channels are intended to be hidden.
