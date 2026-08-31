# Proposal: Board Resistor Dividers & Power Pin Renaming

## Why
Hardware resistor divider ratios for ADC battery voltage sensing are currently represented as opaque `voltage_scale` multipliers in JSON configs rather than physical schematic values. Furthermore, board power management pins on accessory-equipped boards (like `GTRACK`) use legacy names (`BUCK_5V_EN`, `POWER_OUT`) that do not clearly reflect their semantic functional roles (`SERVO_ENABLE` and `PUMP_ENABLE`).

By moving physical resistor definitions (`VOLTAGE_DIV_R_HIGH`, `VOLTAGE_DIV_R_LOW`) into board headers and standardizing power accessory pin names, the codebase gains 1:1 schematic fidelity, eliminates redundant magic numbers from JSON configurations, and provides a clean, optional `calibration_factor` for fine-tuning ADC resistor tolerances.

## What Changes
- **Board Resistor Dividers**: Define physical voltage divider resistors (`VOLTAGE_DIV_R_HIGH`, `VOLTAGE_DIV_R_LOW`) in `struct POWER` across all board headers (`GTRACK.h`, `TRACKLINK_V3.h`, `MIKRO_V2.h`, `TRACKLINK_V2.h`, `BoardBase.h`).
- **Divider Ratio Calculation & Fallback**: Implement a constexpr helper in `BoardBase` computing $(R_{high} + R_{low}) / R_{low}$. If a voltage sense pin is configured but resistors are omitted ($R_{low} \le 0$), default to a standard $2.0$ ratio (1:1 equal divider).
- **Optional Calibration Factor in JSON Config**: Replace mandatory magic `voltage_scale` in hardware JSON configs with an optional `calibration_factor` (multiplier, default `1.0`) and `voltage_offset` (default `0.0`).
- **Power Pin Renames**: Rename `BUCK_5V_EN` to `SERVO_ENABLE` and `POWER_OUT` to `PUMP_ENABLE` in `GTRACK.h`, `BoardBase.h`, `HardwareInit.h`, and board documentation.
- **Hardware Schema & Config Updates**: Update `configs/schemas/hardware_config.schema.json` and board JSON configs to reflect the optional calibration factor.

## Capabilities

### Modified Capabilities
- `board-hardware-abstraction`: Add `VOLTAGE_DIV_R_HIGH`, `VOLTAGE_DIV_R_LOW` to `POWER` struct; rename `BUCK_5V_EN` to `SERVO_ENABLE` and `POWER_OUT` to `PUMP_ENABLE`.
- `battery-protection`: Update voltage sense conversion equation to use board physical divider ratio with optional JSON `calibration_factor` and `voltage_offset`.

## Impact
- **Board definitions**: `boards/BoardBase.h`, `boards/GTRACK.h`, `boards/TRACKLINK_V3.h`, `boards/MIKRO_V2.h`, `boards/TRACKLINK_V2.h`.
- **Core firmware**: `common/Config.h`, `common/ConfigParser.cpp`, `common/HardwareInit.h`, `common/VehicleController.cpp`.
- **Configurations & Schemas**: `configs/hardware_configs/*.json`, `configs/schemas/hardware_config.schema.json`.
- **Tests & Scripts**: `test/host_vc/host_vc_driver.cpp`, `scripts/validate_configs.py`.
