## Context

The RC Brain platform supports comprehensive lighting automation for RC vehicles. Previously, `head_light` was used for both low beam and high beam (switching duty cycles via PWM), while truck fog lights were borrowing `ditch_light` configuration, which conflated truck fog illumination with locomotive alternating ditch light sequencing.

To provide accurate physical modeling for realistic scale trucks and cars, the lighting system must treat low beam (`head_light`), high beam (`full_beam`), and fog lamps (`fog_lamp`) as independent hardware outputs, completely decoupled from locomotive ditch lights.

## Goals / Non-Goals

**Goals:**
- Provide distinct config keys `lights.full_beam` (or `high_beam`) and `lights.fog_lamp` (or `fog_light`) in `hardware-<BOARD>.json`.
- Support dedicated single-pin output (`hardware: "L7"`) for `fog_lamp`.
- Decouple `fog_lamp` from `ditch_light` so `ditch_light` remains exclusively for locomotive alternate-flashing behaviors.
- Energize `head_light` on Low Beam (Bit 0), `full_beam` on High Beam (Bit 1), and `fog_lamp` on Fog Lamp (Bit 2).
- Update the hardware configuration guide and JSON validation schemas.

**Non-Goals:**
- Modifying sound engine audio slots or transmission shift logic.
- Breaking backwards compatibility for existing configs that do not declare `full_beam` or `fog_lamp`.

## Decisions

### 1. Structure in `HardwareConfig::Lights`
In `common/Config.h`:
- `Light fullBeam`: Single pin output for dedicated high beam.
- `Light fogLamp`: Single pin output for dedicated fog lamp.

### 2. Control Surface Mapping
In `common/VehicleController.h`:
- **Low Beam (`bits & 0x01`)**: `HardwareInit::setLight(L.headLight.pin, headBright)`
- **High Beam (`bits & 0x02`)**: If `L.fullBeam.configured`, `HardwareInit::setLight(L.fullBeam.pin, L.fullBeam.brightness)`. If unconfigured, falls back gracefully to driving `head_light` at 100%.
- **Fog Lamp (`bits & 0x04`)**: Energizes `L.fogLamp` pins.
- **Locomotive Ditch Light**: Triggered by Item F / Bit 5 exclusively on locomotive vehicles.

### 3. Documentation Alignment
Update [`GUIDE/HARDWARE_CONFIG.md`](file:///home/sun/Apps/RCKIT/RC_brain/GUIDE/HARDWARE_CONFIG.md) to document `head_light`, `full_beam`, `fog_lamp`, and `ditch_light` as distinct, first-class lighting entries.

## Risks / Trade-offs

- **[Risk] Pin Starvation on Compact Boards**: Some boards (e.g. MIKRO_V2 with 8 light pins) may not have enough free pins for separate low beam, high beam, and dual fog lamps.
  - *Mitigation*: All light outputs remain optional. If `full_beam` is not configured, high beam falls back to stepping `head_light` brightness.
