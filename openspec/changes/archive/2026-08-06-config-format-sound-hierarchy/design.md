## Context

The RC Brain codebase previously used `UPPERCASE` keys in `hardware-config.json` and `vehicle-config.json`, flat hyphenated sound file names (`/sounds/ScaniaV8-idle.json`), and manual drivetrain configuration tags. This led to redundant files, mixed casing, and rigid asset management on LittleFS.

## Goals / Non-Goals

**Goals:**
- Standardize all configuration files to `lower_snake_case`.
- Implement deductive drivetrain topology detection (skid-steer vs. Ackermann).
- Reorganize `/sounds/` into `vehicles/`, `presets/`, and `generic/` directories.
- Implement 3-tier sound resolution (`vehicles/<sound_set>` -> `presets/<preset>` -> `generic/`).
- Update default config assets in `data/` and example configs.

**Non-Goals:**
- Modifying audio PCM sample formats (remains 22,050 Hz, 8-bit PCM JSON arrays).
- Modifying RadioKit BLE protocol packet formats.

## Decisions

1. **`lower_snake_case` Standard**: Standardize all JSON config keys to `lower_snake_case` across hardware and vehicle configs for consistency with modern embedded web API standards.
2. **Deductive Drivetrain Layout**:
   - `left_motor` + `right_motor` under `drivetrain` -> Skid-Steer.
   - `drive_motor` under `drivetrain` -> Ackermann (standard drive + optional steering servo).
3. **Hierarchical 3-Tier Sound Assets**:
   - `/sounds/vehicles/<sound_set>/<slot>.json`
   - `/sounds/presets/<preset>/<slot>.json`
   - `/sounds/generic/<slot>.json`

## Risks / Trade-offs

- **[Backwards Compatibility]**: Older app uploads using `UPPERCASE` keys will fail if fallback parsing isn't included.
  - *Mitigation*: Fall back to upper-case key checks in `ConfigParser.h` if `lower_snake_case` keys are not found during JSON parsing.
