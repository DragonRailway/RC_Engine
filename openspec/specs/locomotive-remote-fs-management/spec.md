## ADDED Requirements

### Requirement: Locomotive remote filesystem management over BLE
The system SHALL support managing and inspecting the locomotive device's LittleFS filesystem through the RadioKit REST Remote API via the Android tablet over BLE.

#### Scenario: Remote filesystem listing
- **WHEN** a client performs `GET /api/fs/list?path=/` against the forwarded RadioKit REST API endpoint while connected to the TRACKLINK_V3 board via BLE
- **THEN** the API returns the LittleFS directory structure containing `/hardware-TRACKLINK_V3.json`, `/vehicle-config.json`, and `/sounds/`

#### Scenario: Remote vehicle config hot-reload
- **WHEN** a new or updated `vehicle-config.json` is uploaded to the device via `POST /api/fs/upload` or `POST /api/fs/write` over BLE
- **THEN** the firmware detects the filesystem write, reloads the configuration via `ConfigParser::loadVehicleConfig`, switches to page 1 ("Loco"), and logs the new profile name and parameters

### Requirement: Locomotive widget control verification over BLE
The system SHALL support executing locomotive controls via RadioKit REST API widget calls on the locomotive page (page 1) over BLE.

#### Scenario: Locomotive engine start and throttle control
- **WHEN** `engine_button` is set to 1 and `throttle_slider` is set to a non-zero value via `PUT /api/widgets/<id>`
- **THEN** the locomotive engine enters the RUNNING state and drives DRIVER_A motor PWM with locomotive mass inertia ramp

#### Scenario: Ditch light and bell activation
- **WHEN** `loco_light` ditch light bit is set and `bell_button` is activated
- **THEN** ditch lights alternate flash on L4/L5 and the EMD bell sound plays
