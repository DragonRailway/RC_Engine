## Why

Skid-steer drivetrains are half-implemented: the parser accepts `left_motor` / `right_motor`, but `HardwareInit` initializes both configs into a **single** `driveMotor` object (the right config silently overwrites the left), and `setSkidMotors(left, right)` only ever applies `leftSpeed`. A vehicle configured for skid-steer drives with one dead track and a config that doesn't match the physical wiring. The `SKID_STEER` path needs to actually drive two independent motor channels.

## What Changes

- **Two-motor skid drive** in `HardwareInit`: the left track runs from `left_motor` (via the existing drive-motor output), the right track runs from `right_motor` via the **second motor output** (`DRIVER_B` H-bridge or an `S*` ESC). Ackermann behavior is untouched.
- **Explicit `drivetrain.type`** config token (`"ackermann"` | `"skid_steer"`) so the drivetrain is declared, not inferred from key presence. Backward compatible: absent token falls back to today's key-presence inference. Unknown value logs a `WARN` and falls back.
- **Skid-steer validation**: `WARN` when `left_motor`/`right_motor` are missing for a skid config; `WARN` when `aux_motor` is configured on a skid board (the aux output is claimed by the right track — the aux channel stays unconfigured, same degraded pattern as `trailer_dcc`).
- **Config plumbing**: `configs/schemas/hardware_config.schema.json` gains `drivetrain.type`; `scripts/build_fs.py` gains a `--hardware <variant>` flag so a board can carry multiple hardware configs (e.g. `hardware-MIKRO_V2-skid.json`) without breaking the unique-match rule.
- **Example config**: `configs/hardware_configs/hardware-MIKRO_V2-skid.json` (left on `DRIVER_A`, right on `DRIVER_B`, differential sensitivity).
- **Tests**: host_vc harness asserts the differential (throttle ± steering·sensitivity), reverse negation, park lock, and brake blending on **both** channels.
- **Docs**: `GUIDE/HARDWARE_CONFIG.md` documents `drivetrain.type`, the skid-steer layout (`left_motor`/`right_motor`/`steering_sensitivity`), and the aux-motor exclusion.

## Capabilities

### New Capabilities

(none — behavior extends existing capabilities)

### Modified Capabilities

- `vehicle-control-loop`: adds the skid-steer differential-drive requirement (two independently driven motor channels, differential mix, park/brake/reverse applied to both).
- `config-reference-guide`: documents the explicit `drivetrain.type` token, the skid-steer motor layout, and the `aux_motor` exclusion in skid mode (replacing the current "presence of `left_motor` selects skid-steer" wording).

## Impact

- `common/Config.h` — no struct changes required (`leftMotor`/`rightMotor` already exist; `drivetrainType` already exists).
- `common/ConfigParser.h` — explicit `type` token parsing + skid validation warnings.
- `common/HardwareInit.h` — the core fix: second motor channel init + `setSkidMotors()` driving both sides.
- `common/VehicleController.h` — differential math already correct; verify only (park/brake handling already covers both sides).
- `configs/schemas/hardware_config.schema.json` — add `drivetrain.type` enum.
- `configs/hardware_configs/` — new `hardware-MIKRO_V2-skid.json` example.
- `scripts/build_fs.py` — `--hardware <variant>` selection.
- `scripts/validate_configs.py` — picks up the new config automatically (CI-able).
- `test/host_vc/` — skid differential tests + EasyKit stubs for the second channel.
- `GUIDE/HARDWARE_CONFIG.md` — skid-steer section.
