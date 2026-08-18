# Proposal: Refactor ConfigParser Defaults to Single Source of Truth

## Why

Currently, default values for hardware parameters (such as `warning_voltage`, `cutoff_voltage`, `boot_latch_s`, `button_hold_s`, `easing_speed_deg_s`, `volume`, `duty.min`, `duty.max`, `steering_servo.frequency`, etc.) are defined in `common/Config.h` inside C++ struct initializers and duplicated in `common/ConfigParser.h` via hardcoded fallback values in ArduinoJson pipe (`|`) expressions.

This dual maintenance creates a risk of default value drift if a default is modified in `Config.h` but forgotten in `ConfigParser.h`.

## What Changes

1. **Refactor `common/ConfigParser.h` Fallbacks**:
   - Replace hardcoded fallback constants in `ConfigParser.h` with their corresponding `config.<field>` struct member references.
   - Standardize fallback expressions: `config.path.field = obj["key"] | obj["KEY"] | config.path.field;`.

2. **Verification & Tests**:
   - Verify config validation script (`python3 scripts/validate_configs.py`).
   - Run host VC physics test harness (`python3 scripts/host_vc_test.py`).
   - Build firmware targets (`TRACKLINK_V3` and `MIKRO_V2`).

## Non-goals

- Altering schema validation default rules in `configs/schemas/hardware_config.schema.json`.
- Modifying any numerical default values in `common/Config.h`.
