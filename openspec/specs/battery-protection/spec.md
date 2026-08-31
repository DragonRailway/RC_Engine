# Battery Protection Specification

## Purpose
Defines requirements for LiPo battery cell detection and low-voltage protection cutoff.
## Requirements
### Requirement: Config-Driven LiPo Cell Count
The system SHALL determine the connected LiPo cell count from the board-specific hardware config (`/hardware-MIKRO_V2.json` or `/hardware-TRACKLINK_V3.json`, selected at compile time by the board define; `battery.cell_count`, 1–4 cells) and establish a low-voltage cutoff threshold equal to the configured `battery.cutoff_voltage` (default 3.3V) per cell.

#### Scenario: Single-cell (1S) pack configured
- **WHEN** the board hardware config declares `battery.cell_count: 1`
- **THEN** the firmware uses 1 cell for cutoff and telemetry regardless of boot voltage (a 1S pack measures 3.0–4.2V, below every 2S+ threshold). The deployed 1S config uses `cutoff_voltage: 3.4` so protection engages before the 3.3V regulator rail drops.

#### Scenario: Fixed pack count configured
- **WHEN** the board hardware config declares `battery.cell_count: N` (2–4)
- **THEN** the firmware uses N cells and establishes the cutoff at `N × cutoff_voltage`

### Requirement: Automatic LiPo Cell Count Detection (Fallback)
When the board hardware config omits `battery.cell_count` (or declares 0), the system SHALL auto-detect the connected LiPo cell count at startup based on measured battery voltage and establish a low-voltage cutoff threshold equal to the configured `cutoff_voltage` (default 3.3V) per cell.

#### Scenario: 1S battery pack connected (fallback)
- **WHEN** battery voltage at boot is less than 4.5V
- **THEN** cell count is set to 1 and cutoff threshold is established at 3.3V

#### Scenario: 2S battery pack connected (fallback)
- **WHEN** battery voltage at boot is between 4.5V and 8.4V
- **THEN** cell count is set to 2 and cutoff threshold is established at 6.6V

#### Scenario: 3S battery pack connected (fallback)
- **WHEN** battery voltage at boot is between 8.4V and 12.6V
- **THEN** cell count is set to 3 and cutoff threshold is established at 9.9V

#### Scenario: 4S battery pack connected (fallback)
- **WHEN** battery voltage at boot is greater than or equal to 12.6V
- **THEN** cell count is set to 4 and cutoff threshold is established at 13.2V

### Requirement: Low Voltage Safety Cutoff and Alarm
The system SHALL continuously monitor battery voltage during operation and enforce a low-voltage cutoff when voltage drops below the threshold for at least 1.5 seconds. Battery telemetry percentage SHALL be computed from the configured per-cell `warning_voltage` (0%) and `full_voltage` (100%, default 4.2V), clamping any voltage at or below the warning threshold to 0%.

#### Scenario: Low voltage threshold reached
- **WHEN** battery voltage remains below the calculated cutoff threshold for 1.5 seconds
- **THEN** the system SHALL stop motor drive output, trigger the out-of-fuel audio sound, and flash hazard lights

#### Scenario: Battery voltage at or below warning level
- **WHEN** battery voltage drops to or below the calculated warning threshold ($V_{\text{bat}} \le \text{warning\_voltage} \times \text{cell\_count}$)
- **THEN** the battery telemetry percentage SHALL report 0%

### Requirement: Board-Driven Voltage Scaling with Optional Config Trim
Battery voltage conversion from ADC pin voltage to battery pack voltage SHALL calculate $V_{\text{bat}} = V_{\text{pin}} \times (\text{DIVIDER\_RATIO} \times \text{calibration\_factor}) + \text{voltage\_offset}$.
`DIVIDER_RATIO` SHALL be obtained from the active board's `POWER::DIVIDER_RATIO`.
`calibration_factor` SHALL be parsed from the hardware JSON config (`battery.calibration_factor`, default `1.0`), allowing unit-level tolerance trim.
`voltage_offset` SHALL be parsed from the hardware JSON config (`battery.voltage_offset`, default `0.0`).

#### Scenario: Default uncalibrated hardware config
- **WHEN** a hardware config omits `calibration_factor` and `voltage_offset`
- **THEN** the firmware uses `calibration_factor = 1.0` and `voltage_offset = 0.0`, computing $V_{\text{bat}} = V_{\text{pin}} \times \text{DIVIDER\_RATIO}$

#### Scenario: Unit-level fine calibration configured
- **WHEN** a hardware config specifies `calibration_factor: 0.985` and `voltage_offset: 0.02`
- **THEN** the firmware applies the calibration trim directly to the board's divider ratio, scaling measured pin voltage accordingly

