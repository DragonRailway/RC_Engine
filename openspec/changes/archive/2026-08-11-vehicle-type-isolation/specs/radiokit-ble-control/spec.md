## MODIFIED Requirements

### Requirement: RadioKit lifecycle
The firmware SHALL call `initRadioKit()` during setup and SHALL call `RadioKit.update()` on every loop iteration. After initialization, the firmware SHALL set the RadioKit device type string from the vehicle config type (`"Truck"` for TRUCK, `"Locomotive"` for LOCOMOTIVE, and the truck string for EXCAVATOR/UNKNOWN) rather than a hardcoded value, and SHALL force the active page to match the vehicle type (page 0 "Truck" for truck-family types, page 1 "Loco" for LOCOMOTIVE) so the app lands on the correct page on connect. The app-initiated page switch remains available to the user afterwards.

#### Scenario: App connects over BLE
- **WHEN** the RadioKit app connects to the device over BLE
- **THEN** the app receives the device name and the active page matches the configured vehicle type; the firmware additionally sets the device type string (`RadioKit.config.type`) from the vehicle config as firmware state (the vendored RadioKit v2.0 lib does not serialize this field, so it is informational until the lib or app supports it)

#### Scenario: Truck config forces Truck page
- **WHEN** the vehicle type is `TRUCK` and the app connects
- **THEN** the active page is page 0 ("Truck") and the device type string is "Truck"

#### Scenario: Loco config forces Loco page
- **WHEN** the vehicle type is `LOCOMOTIVE` and the app connects
- **THEN** the active page is page 1 ("Loco") and the device type string is "Locomotive"

#### Scenario: Loop keeps RadioKit alive
- **WHEN** the firmware loop runs continuously
- **THEN** `RadioKit.update()` is invoked each iteration so widget state changes propagate
