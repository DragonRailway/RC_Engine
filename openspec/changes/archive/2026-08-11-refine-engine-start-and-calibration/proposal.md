## Why

During exploration of the uncommitted working-tree implementation of the engine-start, battery-protection, lighting-automation, and work-machine-hydraulics specs, several decisions were made to tighten behavior against the spec text and fix real conflicts:

- The Engine Start/Stop trigger currently shares control bit 4 (Item E) with the reversing light, so starting the engine also lights the reverse lamp.
- The engine auto-starts when throttle exceeds 10%, contradicting the spec's requirement that a dedicated Engine Start trigger is required.
- Battery voltage calibration is applied from compile-time macros while `hardware-config.json` already carries `voltage_scale`/`voltage_offset` that are parsed but ignored.
- Work-machine auxiliary servo channel 2 is silently dead on TRACKLINK_V3, which has no S3/S4 pins.

This change captures those decisions so the implementation aligns with the specs and the platform reality.

## What Changes

- **Dedicated Engine Power toggle**: Engine Start/Stop moves off the light bitmask entirely — a new latched `engine_start` toggle button widget (ON = run, OFF = stop) drives the engine power state. This matches the latched multi-select widget reality (a level-based bit could never hold RUNNING) and frees the bitmask for lights.
- **Reversing light restored to Item E (bit 4)**: With start no longer sharing the bitmask, the reversing light returns to its original Item E / bit 4 mapping — no new Item F is needed.
- **Strict engine start trigger**: Remove the throttle > 10% auto-start fallback. The engine starts only when the Engine Power toggle is ON; throttle is dead while the engine is OFF or STARTING.
- **Config-driven voltage calibration**: Battery voltage reading uses `voltage_scale` / `voltage_offset` from `hardware-config.json` when present, falling back to the compile-time `VSCALE` / `VOFFSET` macros when absent. Applied consistently across boot cell-count detection, runtime monitoring, and telemetry.
- **Work-machine scope clarification (docs/spec only)**: Aux Servo 2 requires a board with an S3 pin — supported on MIKRO_V2, unsupported on TRACKLINK_V3. Hydraulic input wiring to the RadioKit UI is deferred to a later, user-driven UI edit (firmware keeps the public statics as the input API).

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `engine-start-sound-fx`: engine start requires a dedicated trigger bit distinct from the reversing light; no throttle auto-start.
- `vehicle-control-loop`: battery voltage calibration is sourced from `hardware-config.json` (`voltage_scale`/`voltage_offset`) with compile-time macro fallback.
- `work-machine-hydraulics`: aux servo channel 2 requires an S3 pin (MIKRO_V2 only; TRACKLINK_V3 has no S3/S4); hydraulic UI input wiring is deferred.

## Impact

- `common/VehicleController.h` — control-bit mapping (reversing light), engine start gating, calibration source.
- `common/HardwareInit.h` — no functional change (S3 guard already silently no-ops); comments updated for the board limitation.
- `common/ConfigParser.h` — verify `voltage_scale`/`voltage_offset` parse path is complete (already parsed into `telemetry.vScale`/`vOffset`).
- `boards/TRACKLINK_V3.h`, `boards/MIKRO_V2.h` — unchanged; documented limitation only.
- `src/RADIOKIT.h` — adds the `engine_start` latched toggle widget (Truck page); the user mirrors it in `docs/radiokit-rc-ui-design.json` (type `button`, mode `toggle`, name `engine_start`) and regenerates.
- Config JSONs — no schema change; existing `voltage_scale`/`voltage_offset` fields become effective.
