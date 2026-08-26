## Why

With the truck configuration verified on MIKRO_V2, we need to validate locomotive operation on the `TRACKLINK_V3` board. The board uses `hardware-TRACKLINK_V3-locomotive.json` (drivetrain on DRIVER_A, ditch lights on L4/L5, headlights L1, tail L2, cab L3, step L6) and requires a dedicated locomotive vehicle bundle (`UnionPacific2002` with EMD 16-710 diesel engine sounds). Additionally, we will leverage the connected Android tablet via ADB port forwarding (`adb forward tcp:17007 tcp:7007`) and the RadioKit REST Remote API to upload/manage the LittleFS files and test control widgets over BLE directly from the host environment.

## What Changes

- Add `configs/vehicle_configs/UnionPacific2002/vehicle.json` defining the locomotive profile (type `locomotive`, direct transmission, heavy diesel engine mass simulation `acc: 1, dec: 1`, EMD sounds volume mix).
- Validate and package the `TRACKLINK_V3` locomotive filesystem bundle (`hardware-TRACKLINK_V3-locomotive.json` + `UnionPacific2002` bundle).
- Use the RadioKit REST Remote API via ADB (`tcp:17007`) to verify BLE connection, inspect/manage device LittleFS remotely over BLE (`/api/fs/*`), trigger config hot-reloading, and execute locomotive widget controls (`throttle_slider`, `dir_switch`, `engine_button`, `bell_button`, `loco_light`).

## Capabilities

### New Capabilities
- `locomotive-remote-fs-management`: Testing and managing locomotive LittleFS bundles and verifying locomotive control loop behavior over BLE using the RadioKit Remote REST API on the Android tablet.

### Modified Capabilities
- `config-bundles`: Introduce and validate the `UnionPacific2002` locomotive vehicle bundle (`vehicle.json` with `type: locomotive`) alongside existing sound files.

## Impact

- `configs/vehicle_configs/UnionPacific2002/vehicle.json` created.
- Host scripts/tools interacting with `http://127.0.0.1:17007` for BLE remote filesystem operations and locomotive control tests.
- Verification of TRACKLINK_V3 hardware mappings (DRIVER_A, L1-L6 lights, Ditch light alternation).
