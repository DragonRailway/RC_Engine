# Aux Outputs (aux_motor / aux_light) in the Hardware Config

## Why

Truck auxiliaries — dump-truck tipper, cement-mixer drum, tractor trailer
control — need configurable output channels. Today the only aux output is a
hardcoded pair of servos: `HardwareInit::initAuxServos()` unconditionally
attaches S2 → Aux Servo 1 and S3 → Aux Servo 2 (when the pin exists) with
hardcoded 1000–2000 µs / 50 Hz, and `VehicleController` maps `aux_slider` →
Aux Servo 1. No config surface exists for any of it.

This change makes aux outputs first-class config, following the project
ideology: the **hardware config** describes the physical build ("this rig has
an aux motor wired to DRIVER_B, and it runs a mixer"), while the **vehicle
config stays universal** — it never mentions pins, so the same `vehicle.json`
works on any board.

## What

1. **`aux_motor` in the hardware config** — defined exactly like `drive_motor`
   (`hardware`, `frequency`, `direction`, `duty`) plus a `type` field for its
   purpose. The `hardware` token determines the output kind: `S*` → servo/ESC
   output, `DRIVER_*` → motor-driver output (no separate `aux_servo` key).
   `type`: `mixer` | `tipper` (`trailer_dcc` **deferred**).
2. **`aux_light` in the hardware config** — like `head_light` (`hardware`,
   `brightness_max`).
3. **Behavior wired for `mixer` and `tipper`**, both driven by the app's
   `aux_slider`:
   - `mixer` — slider configured at runtime with **5 detents, no
     self-centering**; the user sets speed + direction and the drum keeps
     running.
   - `tipper` — slider configured with **no detents, self-centering**
     (momentary); position raises/lowers the bed.
   - `RK_Slider` exposes `rk.centering` / `rk.detents` at runtime — the
     firmware sets them in `setup()` per the configured type (verified
     against `lib/rk-arduino`).
4. **Removal of hardcoded aux outputs** — `initAuxServos()` S2/S3 auto-attach
   goes away; aux outputs exist only if declared in config (no legacy
   fallback). The dead `aux_hydraulic2` channel is removed.
5. **Docs** — guide, schema, and the `work-machine-hydraulics` spec updated.

## Non-goals (explicitly deferred)

- **`trailer_dcc`** — the DCC-like trailer control protocol on a driver
  output. The `type` enum reserves it, but the mode is documented as deferred
  and a config using it degrades with a warning until implemented.
- **Excavator/work-machine multi-servo profiles** — the aux surface is
  designed to extend, but boom/arm/bucket mapping is out of scope.
- Migrating `drive_motor` / `steering_servo` / `lights.*` into the aux
  channel system — they stay as-is alongside.
