## MODIFIED Requirements

### Requirement: RadioKit lifecycle
The firmware SHALL initialize RadioKit widgets and pages via `initRadioKit()`, resolve `RadioKit.config.name`, `RadioKit.config.description`, and `RadioKit.config.type` via `applyDeviceMetadata()`, and explicitly commit configuration and start transports in `src/main.cpp` using the following staged sequence:
1. `initRadioKit()`: Configure widget definitions, page names, and orientations without starting transports or clobbering dynamic metadata.
2. `applyDeviceMetadata()`: Resolve `RadioKit.config.name` and `RadioKit.config.description` using the 3-tier cascade:
   - Hardware config top-level `name` and `description` (if present and non-empty).
   - Vehicle config `name` and `description` (`vehicle.name` and `vehicle.description`, if present and non-empty).
   - Fallback defaults (`"RC_UI"` and `""`).
   Set `RadioKit.config.type` to `"Locomotive"` for LOCOMOTIVE and `"Truck"` for truck-family types, setting active page accordingly. Explicitly call `RadioKit.setName(targetName)` to synchronize the NVS name buffer and BLE advertising name with the active profile.
3. `RadioKit.begin()`: Commit the resolved dynamic configuration.
4. `RadioKit.startSerial(Serial)` & `RadioKit.startBLE()`: Start transports broadcasting the resolved name.
5. `RadioKit.enableFS()`: Mount LittleFS for the RadioKit filesystem protocol.

On config hot-reload (`reloadConfigs()`), the firmware SHALL re-evaluate the metadata cascade and update `RadioKit.config.name`, `RadioKit.config.description`, `RadioKit.config.type`, and call `RadioKit.setName(targetName)` to update the active BLE advertising name.

#### Scenario: Clean boot with vehicle name
- **WHEN** the firmware boots with a hardware config that omits `name` and a vehicle config named `"Scania V8"`
- **THEN** `RadioKit.startBLE()` broadcasts the advertised device name as `"Scania V8"` and no subsequent override occurs

#### Scenario: Hardware config overrides vehicle name
- **WHEN** the hardware config defines `"name": "Custom Rig"`
- **THEN** `RadioKit.startBLE()` broadcasts `"Custom Rig"`

#### Scenario: App connects over BLE
- **WHEN** the RadioKit app connects to the device over BLE
- **THEN** the app receives the resolved device name and description matching the priority cascade, and the active page matches the configured vehicle type

#### Scenario: Hardware config overrides vehicle and header name
- **WHEN** the hardware config defines `"name": "Custom Board Name"` and `"description": "Custom Board Desc"`
- **THEN** `RadioKit.config.name` is set to `"Custom Board Name"` and `RadioKit.config.description` is set to `"Custom Board Desc"`, regardless of vehicle config or `RADIOKIT.h`

#### Scenario: Vehicle config fallback when hardware config lacks name
- **WHEN** the hardware config does not define `name` or has an empty string, but the vehicle config defines `"name": "Scania V8"` and `"description": "Heavy Hauler"`
- **THEN** `RadioKit.config.name` is set to `"Scania V8"` and `RadioKit.config.description` is set to `"Heavy Hauler"`
