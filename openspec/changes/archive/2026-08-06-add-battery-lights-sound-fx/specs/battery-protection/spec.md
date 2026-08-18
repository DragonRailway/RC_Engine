## ADDED Requirements

### Requirement: Automatic LiPo Cell Count Detection
The system SHALL auto-detect the connected LiPo cell count (2S, 3S, or 4S) at startup based on measured battery voltage and establish a low-voltage cutoff threshold equal to 3.3V per cell.

#### Scenario: 2S battery pack connected
- **WHEN** battery voltage at boot is less than 8.4V
- **THEN** cell count is set to 2 and cutoff threshold is established at 6.6V

#### Scenario: 3S battery pack connected
- **WHEN** battery voltage at boot is between 8.4V and 12.6V
- **THEN** cell count is set to 3 and cutoff threshold is established at 9.9V

#### Scenario: 4S battery pack connected
- **WHEN** battery voltage at boot is greater than or equal to 12.6V
- **THEN** cell count is set to 4 and cutoff threshold is established at 13.2V

### Requirement: Low Voltage Safety Cutoff and Alarm
The system SHALL continuously monitor battery voltage during operation and enforce a low-voltage cutoff when voltage drops below the threshold for at least 1.5 seconds.

#### Scenario: Low voltage threshold reached
- **WHEN** battery voltage remains below the calculated cutoff threshold for 1.5 seconds
- **THEN** the system SHALL stop motor drive output, trigger the out-of-fuel audio sound, and flash hazard lights
