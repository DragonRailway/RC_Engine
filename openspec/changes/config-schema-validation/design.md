## Context

Configs are parsed by `common/ConfigParser.h` into structs (`common/Config.h`) with zero semantic validation. Boot halts only on missing file / bad JSON; hot-reload keeps the current config on parse failure. Everything semantic (unknown keys, unrecognized `hardware` tokens → `0xFF`, out-of-range values, typo'd enum strings) is silent — the worst case is a dead output with no diagnostic. The RadioKit filesystem manager enables live config edits on-device, so these mistakes happen outside the flash pipeline. The `config-reference-guide` change documented this behavior; this change makes the firmware enforce it with warnings.

## Goals / Non-Goals

**Goals:**
- Every config mistake is surfaced as a `WARN:` line on boot and on hot-reload — no silent degradation.
- Flash-time validation catches mistakes deterministically via a JSON Schema checked in `build_fs.py`, CI-able across all bundles.
- Existing configs remain valid; no format change; behavior (what gets driven) unchanged.

**Non-Goals:**
- Hard-halt on semantic errors (a typo'd pin should not brick a vehicle mid-session). Warn-and-continue only, matching today's degradation but visibly.
- RadioKit app-side surfacing of warnings (serial log only in this change).
- A `"strict": true` opt-in mode (possible future follow-up).
- Rewriting `ConfigParser`'s parse logic — validation is a post-parse pass over the resulting struct plus a key-diff during parse.

## Decisions

- **D1: Warn-and-continue, no new FATALs.** Semantic violations log `WARN:` and keep the parsed (possibly degraded) config. Rationale: preserves current runtime behavior exactly — a bad config does today what it does, but now it says why. Alternative rejected: halt at boot on semantic error — turns typos into bricks.
- **D2: Two-phase validation in ConfigParser.** (1) During parse: unknown-key diff (walk the JSON object tree against the expected key set per section; warn per unknown key; skip intentionally-ignored keys like `turn_light.type` and the case-insensitive legacy variants). (2) Post-parse: struct-level checks (`configured` flags for every light/motor/servo, `duty.min <= duty.max`, 0–100 ranges, frequency bounds, `cell_count` 0–4, `cutoff <= full`, enum strings recognized). One `validateHardwareConfig()` + one `validateVehicleConfig()` entry each.
- **D3: Pin vocabulary check reuses `PinMapper`.** "Unrecognized hardware" = `PinMapper::resolve()` returned `0xFF` where the config expected a pin. Note: `0xFF` is also the "not configured" marker, so the check runs only on keys actually present in the JSON (the parser already tracks presence via `configured` flags — validate against those, not raw struct defaults).
- **D4: Host-side JSON Schema at flash time.** Add `configs/schemas/hardware_config.schema.json` (draft-07; `additionalProperties: false` on each object, `enum` for `direction`/`type` strings, `minimum`/`maximum` for ranges) validated in `build_fs.py` before staging, plus `scripts/validate_configs.py` iterating all bundles for CI. Rationale: deterministic, offline, catches everything including the 76 sound-only bundles. Alternative rejected: firmware-only validation — blind to flash-time mistakes until boot, and can't catch everything a schema can express.
- **D5: Quirks stay quirks.** The validator must match parser reality: `turn_light.type` is accepted-but-ignored (skip in key diff), `brake_light`/`reversing_light` `brightness_max` is accepted-but-ignored (skip or allow in schema), `type` under `drive_motor` is documentation-only. Validation documents reality; it doesn't "clean up" the quirks.
- **D6: Vehicle config gets the same treatment, light touch.** Unknown-key diff + enum checks (`transmission.type`, `vehicle.type`) only — no struct-level range policing beyond what's cheap, since vehicle params are sound-tuning (many have no firmware-enforced bounds today).

## Risks / Trade-offs

- [False-positive warnings on intentional keys] → D5 explicitly whitelists accepted-but-ignored keys; the key-diff shares one allow-list with the parser's own fallback pattern (`snake_case` | `UPPER_CASE`).
- [Warning spam on the serial log] → One consolidated warning per issue, printed once per load; boot + reload only, not per-loop.
- [Schema drift from firmware] → The JSON Schema lives next to the configs and is exercised by `build_fs.py` + CI; the `config-reference-guide` docs already point at `ConfigParser.h` as the authority.
- [Flash/RAM cost] → Negligible: a few dozen lines of checks, no new deps (ArduinoJson already in tree).

## Migration Plan

- No config migration — the format is unchanged and existing configs parse identically. Validation output is additive. Rollback = revert the change.

## Open Questions

- Should warnings also be surfaced to the RadioKit app (e.g. a status field) rather than serial-only? — Deferred (non-goal this change).
- Vehicle schema: full JSON Schema or just the key-diff pass? — D6 chooses light touch; a full vehicle schema can follow.
