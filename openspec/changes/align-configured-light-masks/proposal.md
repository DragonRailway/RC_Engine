## Why

In the RadioKit control UI, unconfigured light buttons are displayed while configured light buttons are hidden on locomotives and road vehicles. This occurs because `HardwareInit::getConfiguredLightMask()` uses an outdated, mismatched bitmask assignment for locomotive controls and incorrectly asserts high beam on vehicles without high beam hardware. Aligning the bitmask calculations ensures only genuinely configured lighting channels are rendered and interactive in the UI.

## What Changes

- Update `HardwareInit::getConfiguredLightMask()` in `common/HardwareInit.h` so that `isLoco == true` maps precisely to the locomotive channels decoded by `VehicleController.h` and the RadioKit UI (Bit 0: Directional Headlight/Tail, Bit 1: Fog Lamp, Bit 2: Ditch Lights, Bit 3: Beacon, Bit 4: Cab Light, Bit 5: Step Light, Bit 6: Aux/Work Light, Bit 7: Hazard).
- Correct locomotive mask generation so unconfigured full beam / high beam flags are not spuriously forced on when only headlights exist.
- Update `GUIDE/HARDWARE_CONFIG.md` documentation to canonicalize the 8-bit multi-select control mappings.
- Extend `test/host_vc/host_vc_driver.cpp` to assert exact `getConfiguredLightMask` output for both truck and locomotive profiles across full and partial hardware configurations.

## Capabilities

### Modified Capabilities

- `vehicle-control-loop`: Update light mask calculation and widget visibility requirements so that only configured light channels are enabled in the 8-bit multi-select widget mask for trucks and locomotives.

## Impact

- `common/HardwareInit.h`: `getConfiguredLightMask()` logic for `isLoco == true` and `isLoco == false`.
- `GUIDE/HARDWARE_CONFIG.md`: Documentation for 8-bit multi-select mapping.
- `test/host_vc/host_vc_driver.cpp`: Unit test assertions for locomotive and truck configured light masks.
- Live device / UI: Correct buttons displayed dynamically for any board hardware configuration.
