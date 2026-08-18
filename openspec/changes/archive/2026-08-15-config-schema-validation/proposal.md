## Why

The hardware and vehicle configs are loaded with zero semantic validation. Today the firmware only fails on missing files or malformed JSON (hard FATAL halt at boot, keep-current at hot-reload). Everything semantic is silent: unknown keys are ignored, a typo'd `hardware` token resolves to `0xFF` ("not configured" — the output simply never turns on), out-of-range values are stored as-is, and a typo'd `direction` silently becomes `FORWARD`. With the RadioKit filesystem manager letting users edit configs live on the phone, a typo looks like a dead output with no diagnostic. The new `GUIDE/Config_Reference.md` documents the schema (unknown keys ignored, unrecognized `hardware` = unconfigured) — this change makes the firmware *enforce* that documentation instead of silently absorbing mistakes.

## What Changes

- **Firmware-side validation** in `common/ConfigParser.h` (warn-and-continue): after parsing a config, validate semantic constraints and log `WARN:` lines for every violation:
  - every `hardware` token resolves to a real pin (not `0xFF` / not configured)
  - duty `min <= max` and both in 0–100; brightness values 0–100; frequencies in sane ranges
  - battery `cell_count` 0–4 (already constrained), `cutoff_voltage <= full_voltage`
  - `direction` and other enum strings are recognized (today a typo silently defaults to `FORWARD`)
  - unknown top-level and nested keys are detected (diff parsed keys against the expected set), warning for each — with care taken so intentional accepted-but-ignored keys (e.g. `turn_light.type`) don't false-positive
- **Host-side JSON Schema** (complement, not replacement): add `configs/schemas/hardware_config.schema.json` (+ vehicle schema if cheap) and validate in `scripts/build_fs.py` before staging, so flash-time mistakes are caught deterministically and all bundles can be validated in CI.
- **Documentation**: update `GUIDE/Config_Reference.md` introduction to describe the new behavior (warnings at boot/reload; strict mode if adopted).

Not changing: boot halt behavior (no new FATALs for semantic issues — warn-and-continue only, matching today's degradation but visibly), sound loading, or the RadioKit hot-reload flow. No `"strict"` mode in this change unless design says otherwise.

## Capabilities

### New Capabilities
- `config-schema-validation`: Firmware logs semantic config warnings (unrecognized hardware, unknown keys, out-of-range values) and host-side JSON Schema validation runs at flash time, so config mistakes are caught instead of silently degrading.

### Modified Capabilities
<!-- None — this adds validation behavior; existing specs (config-filesystem-management, config-reference-guide) are unaffected. -->

## Impact

- **Firmware**: `common/ConfigParser.h` (validation pass after parse), possibly `common/Config.h` (no struct changes expected).
- **Scripts**: `scripts/build_fs.py` (schema validation before staging), new `scripts/validate_configs.py` (validate all bundles, CI-able).
- **Docs**: `GUIDE/Config_Reference.md` (validation behavior), `AGENTS.md` if it documents config loading.
- **New files**: `configs/schemas/hardware_config.schema.json` (+ optional vehicle schema).
- No change to config file format — existing configs stay valid.
