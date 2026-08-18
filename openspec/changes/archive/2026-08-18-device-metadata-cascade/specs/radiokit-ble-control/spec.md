## MODIFIED Requirements

### Requirement: RadioKit lifecycle
The firmware SHALL call `initRadioKit()` during setup (configuring initial defaults and starting BLE and Serial transports plus the filesystem feature) and SHALL call `RadioKit.update()` on every loop iteration. The firmware SHALL resolve `RadioKit.config.name` and `RadioKit.config.description` using a 3-tier cascade:
1. Top-level `name` and `description` from the board's hardware config JSON (if present and non-empty).
2. `name` and `description` from the vehicle config JSON (`vehicle.name` and `vehicle.description`, if present and non-empty).
3. The fallback defaults defined in `RADIOKIT.h` (`initRadioKit()`).

After initialization, the firmware SHALL set the RadioKit device type string from the vehicle config type (`"Truck"` for TRUCK, `"Locomotive"` for LOCOMOTIVE, and the truck string for EXCAVATOR/UNKNOWN) rather than a hardcoded value, and SHALL force the active page to match the vehicle type (page 0 "Truck" for truck-family types, page 1 "Loco" for LOCOMOTIVE) so the app lands on the correct page on connect. On config hot-reload, the firmware SHALL re-evaluate the metadata cascade and update `RadioKit.config.name`, `RadioKit.config.description`, and runtime BLE advertising name accordingly.

#### Scenario: App connects over BLE
- **WHEN** the RadioKit app connects to the device over BLE
- **THEN** the app receives the resolved device name and description matching the priority cascade, and the active page matches the configured vehicle type

#### Scenario: Hardware config overrides vehicle and header name
- **WHEN** the hardware config defines `"name": "Custom Board Name"` and `"description": "Custom Board Desc"`
- **THEN** `RadioKit.config.name` is set to `"Custom Board Name"` and `RadioKit.config.description` is set to `"Custom Board Desc"`, regardless of vehicle config or `RADIOKIT.h`

#### Scenario: Vehicle config fallback when hardware config lacks name
- **WHEN** the hardware config does not define `name` or has an empty string, but the vehicle config defines `"name": "Scania V8"` and `"description": "Heavy Hauler"`
- **THEN** `RadioKit.config.name` is set to `"Scania V8"` and `RadioKit.config.description` is set to `"Heavy Hauler"`

#### Scenario: RADIOKIT.h fallback when both configs lack metadata
- **WHEN** neither hardware config nor vehicle config specifies `name` or `description`
- **THEN** `RadioKit.config.name` falls back to the default in `RADIOKIT.h` (e.g. `"RC_UI"`) and `RadioKit.config.description` falls back to `""`

#### Scenario: Metadata updates on hot-reload
- **WHEN** a config file is modified and reloaded via LittleFS watcher
- **THEN** the device re-evaluates the cascade and updates `RadioKit.config.name`, `RadioKit.config.description`, and the active BLE advertising name

#### Scenario: Truck config forces Truck page
- **WHEN** the vehicle type is `TRUCK` and the app connects
- **THEN** the active page is page 0 ("Truck") and the device type string is "Truck"

#### Scenario: Loco config forces Loco page
- **WHEN** the vehicle type is `LOCOMOTIVE` and the app connects
- **THEN** the active page is page 1 ("Loco") and the device type string is "Locomotive"

#### Scenario: Loop keeps RadioKit alive
- **WHEN** the firmware loop runs continuously
- **THEN** `RadioKit.update()` is invoked each iteration so widget state changes propagate
