# Spec: Use uint16_t for Power Time Parameters in Seconds

## ADDED Requirements

### Requirement: Integer seconds representation for power time parameters

The hardware configuration schema (`configs/schemas/hardware_config.schema.json`) and C++ configuration struct (`HardwareConfig::Power` in `common/Config.h`) SHALL represent power time parameters in integer seconds using `uint16_t`:
- `boot_latch_s`: `uint16_t`, default `1` (range `0`–`30`s)
- `button_hold_s`: `uint16_t`, default `4` (range `1`–`30`s)
- `disconnect_timeout_s`: `uint16_t`, default `60` (range `0`–`3600`s; 0 disables auto-off)
- `warning_window_s`: `uint16_t`, default `10` (range `1`–`60`s)
- `cutoff_delay_s`: `uint16_t`, default `2` (range `0`–`60`s)

#### Scenario: Integer power time parameters parsed from JSON
- **WHEN** hardware config specifies `"power": { "boot_latch_s": 2, "button_hold_s": 5, "disconnect_timeout_s": 120, "warning_window_s": 15, "cutoff_delay_s": 2 }`
- **THEN** `ConfigParser` populates `HardwareConfig::Power` with integer `uint16_t` values (2, 5, 120, 15, 2)
