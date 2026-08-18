## Context

The working tree contains an uncommitted implementation of four archived specs (engine-start-sound-fx, battery-protection, advanced-lighting-automation, work-machine-hydraulics) in `common/VehicleController.h` and `common/HardwareInit.h`. Exploration identified three behavior conflicts with the spec text and platform reality:

1. Engine Start/Stop shares control bit 4 (Item E) with the reversing light; start is also auto-triggered by throttle > 10%.
2. Battery voltage calibration comes from compile-time `VSCALE`/`VOFFSET` macros, while `hardware-config.json` already carries `voltage_scale`/`voltage_offset` (currently `1.0`/`0.0`) that are parsed but never used.
3. Aux Servo 2 binds to `S3`, which does not exist on TRACKLINK_V3 (only S1/S2) — the attach silently no-ops.

The RadioKit widgets (`led_select`, `lights_toggle`) currently expose exactly 5 items (A–E, bits 0–4), so a new bit is required for the reversing light.

## Goals / Non-Goals

**Goals:**
- Dedicate bit 4 (Item E) to Engine Start/Stop only; reversing light moves to its own dedicated bit.
- Strict per-spec engine start: no throttle auto-start fallback.
- Make `voltage_scale`/`voltage_offset` from `hardware-config.json` authoritative for all battery math, with compile-time macro fallback when absent.
- Document the Aux Servo 2 board limitation (MIKRO_V2 only) without changing hardware behavior.

**Non-Goals:**
- No RadioKit RC_UI design changes in this change — the user edits the design JSON manually (Item F addition is tracked separately).
- No new hardware-config schema fields — `voltage_scale`/`voltage_offset` already exist.
- No wiring of hydraulic UI inputs — the public statics remain the input API for a later, user-driven UI binding.

## Decisions

### D1. Dedicated latched Engine Power toggle widget
Engine start/stop is driven by a dedicated latched `RK_ToggleButton` (`engine_start`) added to the Truck page: ON starts the engine (OFF → STARTING → RUNNING), OFF stops it (RUNNING → OFF, and cancels a crank in progress). This matches the widget reality — `led_select`/`lights_toggle` are latched multi-select toggles, so a level-based bit trigger could never hold RUNNING (it would oscillate start→stop). With start moved off the bitmask, the reversing light returns to its original Item E / bit 4 mapping; no Item F is needed.

- *Alternatives considered*: keep bit 4 as a level-based start trigger — rejected: with a latched widget the engine would crank, reach RUNNING, and immediately stop again. Edge-triggered toggle on a latched bit — rejected: a 3-tap on→off→on cycle is needed to stop, awkward UX.

### D2. Strict engine start
Remove `throttlePct > 10` from the OFF → STARTING transition. The Engine Start bit becomes the sole trigger; the existing gating (motor commands only when RUNNING) already satisfies the "no throttle while OFF/STARTING" requirement.

### D3. Config-driven voltage calibration
All battery math (boot cell-count detection, runtime low-voltage monitor, telemetry percent) uses `s_hw->telemetry.vScale` / `s_hw->telemetry.vOffset`. `ConfigParser.h` changes its parse defaults from `1.0f`/`0.0f` to the compile-time `VSCALE`/`VOFFSET` macros (guarded with `#ifndef`), so config wins when present and macros apply when absent. The shipped `data/hardware-config.json` is updated to carry the board's calibrated values (`1.8`/`-0.2`) so behavior is unchanged on fresh deploys.

- *Alternatives considered*: keep macros authoritative and ignore the JSON fields. Rejected — the user decided config should win, and the config is the user-tunable surface.

### D4. Aux Servo 2 board support
No functional change: `PinMapper::resolve("S3")` returns `0xFF` on TRACKLINK_V3 and the existing guard already skips attach. Update comments in `HardwareInit.h` to state the MIKRO_V2-only availability; the work-machine-hydraulics spec delta records the requirement.

### D5. Hydraulic input API surface
The public statics (`aux_hydraulic1`, `aux_hydraulic2`, `bucket_rattle_trigger`, `dump_bed_toggle`) remain the firmware's input contract for external UI bindings. No changes; documented in the work-machine-hydraulics spec delta.

## Risks / Trade-offs

- [Item F missing from UI] Until the user adds Item F to the RC_UI design, the reversing light is uncontrollable from the UI → it simply stays off (safe default). The firmware bit contract is documented and the UI edit is a tracked todo.
- [Config calibration changes on-device readings] Devices with an existing `hardware-config.json` on LittleFS (containing `1.0`/`0.0`) will switch to those values once config wins. → Ship the updated `hardware-config.json`; the config hot-reload path makes pushing it to a device a non-flash operation.
- [`VSCALE`/`VOFFSET` undefined in some build context] Both PlatformIO envs define them, but guard the ConfigParser default with `#ifndef` and fall back to `1.0f`/`0.0f` to keep any unusual build from breaking.

## Migration Plan

1. Update `data/hardware-config.json` `telemetry` block to `voltage_scale: 1.8`, `voltage_offset: -0.2` (mirrors current macro calibration).
2. Firmware changes are compile-time; deploy via normal `pio run -t upload` + config push.
3. UI design JSON (`docs/radiokit-rc-ui-design.json`) updated by the user in a separate step to add Item F; regenerate `src/RADIOKIT.h` from it (or apply the matching item-5 edit) so the bit contract lines up.

## Open Questions

- Whether Item F should live on the same multi-select widget (recommended) or a new widget — the user decides during the manual UI edit; the firmware contract is bit 5 either way.
