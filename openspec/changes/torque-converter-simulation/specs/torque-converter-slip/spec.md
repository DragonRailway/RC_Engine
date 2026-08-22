## ADDED Requirements

### Requirement: Engine Load Tracking
The sound engine SHALL compute an instantaneous engine load value based on the difference between commanded throttle and current engine RPM, bounded between 0 and 180.

#### Scenario: Throttle applied from idle
- **WHEN** throttle is rapidly increased while current engine RPM is low
- **THEN** engine load increases proportionally up to 180

#### Scenario: RPM caught up with throttle
- **WHEN** current engine RPM catches up to or exceeds commanded throttle
- **THEN** engine load returns to 0

### Requirement: Load-Dependent Torque Converter Slip
When automatic transmission mode (`TRANS_AUTOMATIC`) is active, the engine SHALL compute torque converter slip proportional to engine load and `torqueConverterSlip` percentage, adding this slip to the base gear-ratio target RPM.

#### Scenario: Acceleration in 2nd or higher gear
- **WHEN** accelerating with high engine load in gear 2 or above
- **THEN** converter slip equals `engineLoad * torqueConverterSlip / 100` and flares target RPM

#### Scenario: Launch in 1st or Reverse gear
- **WHEN** accelerating from standstill in 1st or Reverse gear
- **THEN** converter slip applies a 2x launch multiplier (`engineLoad * torqueConverterSlip / 100 * 2`) to simulate hydraulic stall speed

#### Scenario: Steady cruising speed
- **WHEN** vehicle speed stabilizes and engine load reaches 0
- **THEN** converter slip reaches 0 and engine RPM locks to the direct gear ratio speed

### Requirement: Torque Converter Slip Configuration
The vehicle configuration parser SHALL parse `torque_converter_slip` / `TORQUE_CONVERTER_SLIP` percentage (default 100) from the `transmission` JSON block.

#### Scenario: Custom torque converter slip in JSON
- **WHEN** `vehicle-config.json` contains `"transmission": { "type": "AUTOMATIC", "torque_converter_slip": 80 }`
- **THEN** the sound engine configures `torqueConverterSlip = 80`
