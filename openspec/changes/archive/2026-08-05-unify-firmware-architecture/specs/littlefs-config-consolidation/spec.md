# LittleFS Config Consolidation

## MODIFIED Requirements

### Requirement: Vehicle type as runtime JSON config
Vehicle type SHALL be loaded dynamically at runtime from `/vehicle-config.json` via the `"VEHICLE": { "TYPE": "..." }` field, defaulting to `TRUCK` if omitted.

#### Scenario: Truck vehicle type parsed
- **WHEN** `/vehicle-config.json` contains `"VEHICLE": { "TYPE": "TRUCK" }`
- **THEN** `config.type` is set to `VehicleType::TRUCK`

#### Scenario: Locomotive vehicle type parsed
- **WHEN** `/vehicle-config.json` contains `"VEHICLE": { "TYPE": "LOCOMOTIVE" }`
- **THEN** `config.type` is set to `VehicleType::LOCOMOTIVE`

#### Scenario: Missing TYPE field defaults to TRUCK
- **WHEN** `/vehicle-config.json` does not contain a `TYPE` field
- **THEN** `config.type` defaults to `VehicleType::TRUCK`
