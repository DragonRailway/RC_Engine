# Change Proposal: Locomotive Independent Lighting & UI Sync

## Why

In the current firmware, locomotive lighting controls inside `VehicleController.h` retained legacy truck lighting logic that interlocked the first two buttons (Headlight Bit 0 and High Beam Bit 1), causing Bit 1 to be forcibly cleared if Bit 0 was OFF. Furthermore, ditch lights were coupled to horn/bell triggers rather than being purely manual, and the local UI design file was not synced with the latest `RC_UI` design from the RadioKit mobile app.

## What Changes

1. **Decouple Locomotive Lighting Controls**:
   - Remove truck-style Headlight/High Beam interlock (`loco_light.rk.value &= ~0x02`) from the locomotive branch.
   - Master Headlight (Bit 0) acts as a simple ON/OFF switch coupled to `dir_switch` (Forward -> Front `L1`, Reverse -> Rear `L2`).
   - Ditch Lights (Bit 2) operate purely manual without automatic bell/horn triggering.
   - All 8 buttons in `loco_light` operate independently without cross-button side-effects.

2. **Sync UI Design & Header**:
   - Update `docs/radiokit-rc-ui-design.json` and `src/RADIOKIT.h` with the latest `rc_ui_default` fetched from the mobile app.

3. **Align Tests**:
   - Update `test/host_vc/host_vc_driver.cpp` and `scripts/host_vc_test.py` to verify independent button states and manual ditch light operation.

## Capabilities

### Modified Capabilities
- `locomotive-directional-lighting`: Simplified master directional switch without interlocks.
- `locomotive-lighting-effects`: Purely manual ditch light operation without bell/horn coupling.

## Impact

- `common/VehicleController.h`: Simplified locomotive lighting state handler.
- `docs/radiokit-rc-ui-design.json`: Latest UI layout from Android tablet.
- `src/RADIOKIT.h`: Generated RadioKit header.
- `test/host_vc/host_vc_driver.cpp`: Host tests for locomotive lights.
