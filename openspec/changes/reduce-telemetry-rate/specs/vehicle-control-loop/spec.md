## MODIFIED Requirements

### Requirement: Telemetry reporting
The firmware SHALL update `telemetry_Battery` with battery voltage converted to percent and `telemetry_Speed` with an estimated speed derived from throttle, at a **1000ms interval** during active operation. Battery voltage SHALL be computed using `voltage_scale` / `voltage_offset` from the board-specific hardware config (`/hardware-MIKRO_V2.json` or `/hardware-TRACKLINK_V3.json`) when present, falling back to the board's compile-time `VSCALE` / `VOFFSET` calibration values when absent.

#### Scenario: Battery telemetry updated
- **WHEN** the control loop runs
- **THEN** `telemetry_Battery.rk.content` holds a percentage string based on the measured voltage

#### Scenario: Speed telemetry updated
- **WHEN** the control loop runs
- **THEN** `telemetry_Speed.rk.content` holds a speed estimate derived from throttle and direction updated at a **1000ms interval**

#### Scenario: Config calibration applied
- **WHEN** the board hardware config contains `voltage_scale` and `voltage_offset`
- **THEN** battery percentage is computed from the configured calibration values

#### Scenario: Config calibration absent
- **WHEN** the board hardware config does not contain `voltage_scale` or `voltage_offset`
- **THEN** battery percentage is computed using the board's compile-time `VSCALE` / `VOFFSET` calibration
