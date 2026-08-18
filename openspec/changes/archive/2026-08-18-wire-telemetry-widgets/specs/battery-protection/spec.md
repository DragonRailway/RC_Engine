## MODIFIED Requirements

### Requirement: Low Voltage Safety Cutoff and Alarm
The system SHALL continuously monitor battery voltage during operation and enforce a low-voltage cutoff when voltage drops below the threshold for at least 1.5 seconds. Battery telemetry percentage SHALL be computed from the configured per-cell `warning_voltage` (0%) and `full_voltage` (100%, default 4.2V), clamping any voltage at or below the warning threshold to 0%.

#### Scenario: Low voltage threshold reached
- **WHEN** battery voltage remains below the calculated cutoff threshold for 1.5 seconds
- **THEN** the system SHALL stop motor drive output, trigger the out-of-fuel audio sound, and flash hazard lights

#### Scenario: Battery voltage at or below warning level
- **WHEN** battery voltage drops to or below the calculated warning threshold ($V_{\text{bat}} \le \text{warning\_voltage} \times \text{cell\_count}$)
- **THEN** the battery telemetry percentage SHALL report 0%
