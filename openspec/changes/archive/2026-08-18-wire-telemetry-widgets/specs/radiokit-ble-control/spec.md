## MODIFIED Requirements

### Requirement: Telemetry widget registration
The firmware SHALL register `telemetry_Battery` (unit "%") and `telemetry_Speed` (unit "km/h") output widgets, and continuously update their string buffers during operation at a 250ms interval.

#### Scenario: Telemetry visible in app
- **WHEN** the app displays the device
- **THEN** Battery and Speed telemetry readouts are present with their initial "--" content

#### Scenario: Real-time telemetry streaming
- **WHEN** the vehicle controller runs its telemetry update cycle
- **THEN** `telemetry_Battery` is formatted as integer percentage (0–100) and `telemetry_Speed` is formatted as km/h (0–200, mapped as `abs(motorSpeed) * 2`)
