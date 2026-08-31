## ADDED Requirements

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
