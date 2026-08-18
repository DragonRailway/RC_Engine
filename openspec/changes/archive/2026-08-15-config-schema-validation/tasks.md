## 1. Firmware validation pass (ConfigParser)

- [x] 1.1 Add unknown-key detection to `loadHardwareConfig`: walk top-level + nested JSON objects against the expected key set (honoring `snake_case` | `UPPER_CASE` variants and accepted-but-ignored keys), logging `WARN:` per unknown key
- [x] 1.2 Add unknown-key detection to `loadVehicleConfig` (top-level sections + `vehicle` object), same rules
- [x] 1.3 Add `validateHardwareConfig()` post-parse checks: every present light/motor/servo has a resolved pin (not `0xFF`), duty `min <= max` and both 0–100, brightness 0–100, frequency sanity bounds, battery `cell_count` 0–4 and `cutoff <= full`
- [x] 1.4 Add enum-string recognition checks for `direction` (and `type` where read) — warn on unrecognized values instead of silently defaulting
- [x] 1.5 Add `validateVehicleConfig()` post-parse checks (light touch): enum checks for `vehicle.type` and `transmission.type`
- [x] 1.6 Wire validation into boot (`setup()`) and hot-reload (`reloadConfigs()`) so warnings print on both paths

## 2. Host-side JSON Schema + tooling

- [x] 2.1 Author `configs/schemas/hardware_config.schema.json` (draft-07): all sections, `additionalProperties: false`, enums for `direction`/`type`, numeric `minimum`/`maximum`, quirks allowed (turn_light.type, brake/reversing brightness_max, reversing light aliases)
- [x] 2.2 Validate the schema in `scripts/build_fs.py` before staging — exit nonzero with violations on failure
- [x] 2.3 Add `scripts/validate_configs.py` that validates all hardware configs and vehicle bundles in the repo (CI-able); run it against the repo and fix any genuine violations found in shipped configs

## 3. Docs

- [x] 3.1 Update `GUIDE/HARDWARE_CONFIG.md` introduction to describe the new validation behavior (warnings at boot/reload, flash-time schema check)
- [x] 3.2 Update `AGENTS.md` if it documents config loading/validation

## 4. Verification

- [x] 4.1 Host tests: `validate_configs.py` passes all bundles; `build_fs.py` rejects an intentionally-broken config with clear violations
- [x] 4.2 Firmware: build both envs (`TRACKLINK_V3`, `MIKRO_V2`) and run host VC tests — no regressions
- [x] 4.3 Firmware warning check: boot log shows no warnings for the shipped MIKRO_V2 config; a deliberately broken config produces the expected `WARN:` lines on boot and on hot-reload
