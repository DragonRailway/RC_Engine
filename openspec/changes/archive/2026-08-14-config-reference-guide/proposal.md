## Why

The hardware config schema (`configs/hardware_configs/hardware-<BOARD>.json`) is defined only by the firmware's parser (`common/ConfigParser.h`) and struct defaults (`common/Config.h`) — there is no user-facing documentation. Because the firmware performs no schema validation (unknown keys are silently ignored and a typo'd `hardware` name resolves to `0xFF` = "not configured"), the config is effectively self-documenting in the worst way: users must read C++ headers to understand it. We want a Klipper-style configuration reference (like https://www.klipper3d.org/Config_Reference.html) so configuring a board is a lookup, not archaeology.

## What Changes

- Add `GUIDE/Config_Reference.md`: a single-page, Klipper-style reference for the **hardware config** (`hardware-<BOARD>.json`), with a table of contents and anchor links.
- Each config section (`sound`, `drivetrain`, `lights`, `animation`, `telemetry`, `battery`) documented Klipper-style: heading, one-line purpose, then "The following parameters are available in the `X` section:" with **name · type · default · allowed values · notes** for every parameter.
- Add a **Pin Reference** section: per-board tables (MIKRO_V2, TRACKLINK_V3) mapping the `hardware` tokens (`L0..L8`, `S1..S4`, `HBRIDGE_A`/`HBRIDGE_B`) to physical GPIOs, including bridge pin assignments and the shared `HBRIDGE_*` semantic markers.
- Document quirks and accepted-but-ignored keys explicitly (e.g. `turn_light.type`, forced `brightness_max` on brake/reversing lights, case-insensitive legacy keys, the Ackermann/Skid-steer `drivetrain` fork, `reversing_light` light-alias values).
- Add `GUIDE/README.md` as an index, noting that a vehicle-config reference is a future stub.
- No firmware, script, or config behavior changes.

## Capabilities

### New Capabilities
- `config-reference-guide`: Documentation of the hardware config schema as a Klipper-style reference, covering all config sections, parameters (with types/defaults/allowed values), per-board pin vocabulary, and documented quirks.

### Modified Capabilities
<!-- None — documentation only, no spec-level behavior changes. -->

## Impact

- **Docs only**: `GUIDE/README.md` (new), `GUIDE/Config_Reference.md` (new).
- No changes to firmware (`common/`, `src/`, `boards/`), scripts, configs, or specs — the guide is descriptive, not prescriptive.
- Source of truth for the guide: `common/Config.h` (defaults), `common/ConfigParser.h` (parsed keys), `common/PinMapper.h` + `boards/*.h` (pin vocabulary), and the two shipped configs (`configs/hardware_configs/*.json`).
