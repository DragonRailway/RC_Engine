## Context

The hardware config (`configs/hardware_configs/hardware-<BOARD>.json`, deployed to `/hardware-config.json`) is parsed at boot by `common/ConfigParser.h` into the `HardwareConfig` struct (`common/Config.h`). Pin `hardware` strings are resolved by `common/PinMapper.h` against the compile-time board (`-D MIKRO_V2` / `-D TRACKLINK_V3`), whose physical GPIOs live in `boards/*.h`.

There is currently no user-facing config documentation. The de-facto schema is the parser itself, and the firmware applies no validation: unknown keys are ignored, missing keys fall back to struct defaults, and an unrecognized `hardware` token resolves to `0xFF` (light/motor silently unconfigured). The user asked for a Klipper-style configuration reference (https://www.klipper3d.org/Config_Reference.html) so that configuring a board is a lookup rather than header-reading.

## Goals / Non-Goals

**Goals:**
- A single Klipper-style `GUIDE/Config_Reference.md` covering the hardware config end-to-end.
- Document every parameter the parser reads: name, type, default, allowed values, notes.
- Per-board pin reference (`L0..`, `S1..`, `HBRIDGE_*` tokens → physical GPIOs) for MIKRO_V2 and TRACKLINK_V3.
- Explicitly document quirks, accepted-but-ignored keys, and legacy fallbacks, so the guide is trustworthy as the de-facto schema.
- A minimal `GUIDE/README.md` index.

**Non-Goals:**
- Vehicle config (`vehicle.json`, sound engine params) — explicitly deferred to a future `Vehicle_Config_Reference.md` stub.
- Any firmware, script, or behavior change. The guide describes what exists; it does not alter parsing or defaults.
- Schema validation tooling (e.g., a JSON-schema validator) — out of scope; a risk noted below.

## Decisions

- **D1: Single-file reference, Klipper layout.** One `GUIDE/Config_Reference.md` with a table of contents and anchor links, section-by-section (sound, drivetrain, lights, animation, telemetry, battery), then a Pin Reference, then a worked example. Rationale: mirrors Klipper's Config_Reference.html exactly and is greppable in one file. Alternative rejected: split per-area files — more navigation for a schema this size.
- **D2: Parameter format.** Each section: heading, one-line purpose, then "The following parameters are available in the `X` section:" with a consistent per-parameter bullet: `name` — type · default · allowed values, then a one-line note where non-obvious. Defaults taken from `common/Config.h` struct initializers (the authoritative source).
- **D3: Sectional mapping matches the parser, not the JSON shape alone.** The `drivetrain` section is documented as a fork: `left_motor`/`right_motor` present ⇒ SKID_STEER (with `steering_sensitivity`), otherwise `drive_motor` + `steering_servo` ⇒ ACKERMANN. `drive_motor`/`left_motor`/`right_motor` share one parameter block (same struct type).
- **D4: Pin Reference is per-board.** Two tables (MIKRO_V2, TRACKLINK_V3) mapping every resolvable token to its GPIO, plus bridge pin assignments (PWM1/PWM2/DIR/EN/BEMF). Rationale: the same token means different GPIOs per board (`L1` = 38 on MIKRO_V2, 6 on TRACKLINK_V3) — the single most error-prone surface for users. Also documents `HBRIDGE_A`/`HBRIDGE_B` as *semantic* markers (resolved per-board, not GPIO literals).
- **D5: Quirks documented inline + a dedicated "Quirks & Legacy" notes in each section.** Accepted-but-ignored keys (`turn_light.type`), forced values (`brake_light`/`reversing_light` brightness = 100), the `reversing_light` light-alias values (`"head_light"`/`"tail_light"`/`"brake_light"`), case-insensitive legacy keys (`sound`|`SOUND`), and the no-validation caveat all get explicit callouts so readers are never misled.
- **D6: Source-of-truth check before writing.** Every documented default, range, and allowed value is verified against `Config.h`/`ConfigParser.h`/`PinMapper.h`/`boards/*.h` and the two shipped configs; the example config is a copy of the shipped MIKRO_V2 config annotated.

## Risks / Trade-offs

- [Docs drift from code] → The reference is derived from headers, not generated; future parser changes may leave the guide stale. Mitigation: a note in the guide pointing at the authoritative sources (`common/Config.h`, `common/ConfigParser.h`), and the change is docs-only so drift is visible in review.
- [Quirk documentation may read as endorsement] → Each quirk is framed as "accepted but ignored / forced by firmware" with the actual behavior stated, matching parser reality.
- [No schema validation exists] → Documented loudly in the introduction ("unknown keys are silently ignored; a typo'd `hardware` = unconfigured output"), turning the guide into the closest thing to a schema. A validation tool remains a possible future change.

## Migration Plan

- N/A — documentation only. No deployment, rollback, or config migration. The guide can be merged independently of firmware changes.

## Open Questions

- None blocking. (Vehicle-config reference intentionally deferred per user decision.)
