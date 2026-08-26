# Design: Locomotive Independent Lighting & UI Sync

## Architecture Overview

1. **Locomotive Light State Handler**:
   In `VehicleController.h`, when `isLoco` is true:
   - `s_headlightMode = (bits & 0x01) ? 1 : 0;`
   - No modification to `loco_light.rk.value`.
   - Each bit in `loco_light` (0..7) is an independent channel toggle.

2. **Hardware Channel Mapping**:
   - `head_light` (`L1`): Driven by Directional Headlight when moving Forward (`dir_switch` = 1) and Bit 0 is ON.
   - `tail_light` (`L2`): Driven by Directional Headlight when moving Reverse (`dir_switch` = 0) and Bit 0 is ON.
   - `ditch_light` (`L4`/`L5`): Driven by Bit 2 (`0x04`) purely manual cross-fade.
   - `cab_light` (`L6`): Driven by Bit 4 (`0x10` / `cable-car`).
   - `step_light` (`L7`): Driven by Bit 5 (`0x20` / `tablet`).

3. **UI Design Sync**:
   - Sync `docs/radiokit-rc-ui-design.json` and `src/RADIOKIT.h` with latest design from app.
