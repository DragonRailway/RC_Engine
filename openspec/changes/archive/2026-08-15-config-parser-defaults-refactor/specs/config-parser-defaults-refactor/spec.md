# Spec: ConfigParser Defaults Single Source of Truth

## ADDED Requirements

### Requirement: Single source of truth for hardware config defaults

The firmware config parser (`common/ConfigParser.h`) SHALL reference the default-initialized member fields of `HardwareConfig` as fallback values for missing JSON keys instead of hardcoding duplicate numeric or string literal defaults.

#### Scenario: Missing optional field uses Config.h default
- **WHEN** a hardware config JSON omits an optional field (e.g. `boot_latch_s` in `"power"`)
- **THEN** `ConfigParser` retains the default value initialized in `HardwareConfig` (`common/Config.h`)

#### Scenario: Specified field overrides default
- **WHEN** a hardware config JSON specifies an explicit value for an optional field
- **THEN** `ConfigParser` overwrites the default with the explicit value from JSON
