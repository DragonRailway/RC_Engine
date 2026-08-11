## MODIFIED Requirements

### Requirement: Telemetry reporting
The firmware SHALL update `telemetry_Battery` with battery voltage converted to percent and `telemetry_Speed` with an estimated speed derived from throttle, at a bounded rate. Battery voltage SHALL be computed using `voltage_scale` / `voltage_offset` from `hardware-config.json` when present, falling back to the board's compile-time `VSCALE` / `VOFFSET` calibration values when absent.

#### Scenario: Battery telemetry updated
- **WHEN** the control loop runs
- **THEN** `telemetry_Battery.rk.content` holds a percentage string based on the measured voltage

#### Scenario: Speed telemetry updated
- **WHEN** the control loop runs
- **THEN** `telemetry_Speed.rk.content` holds a speed estimate derived from throttle and direction

#### Scenario: Config calibration applied
- **WHEN** `hardware-config.json` contains `voltage_scale` and `voltage_offset`
- **THEN** battery percentage is computed from the configured calibration values

#### Scenario: Config calibration absent
- **WHEN** `hardware-config.json` does not contain `voltage_scale` or `voltage_offset`
- **THEN** battery percentage is computed using the board's compile-time `VSCALE` / `VOFFSET` calibration
