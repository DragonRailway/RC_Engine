# RadioKit BLE Control Specification

## Purpose
Defines requirements for RadioKit BLE stack integration, widget control surfaces, and remote API interactions.

## Requirements


### Requirement: RadioKit library integration
The build SHALL include the RadioKit Arduino library v2.0.0 (local symlink to `/home/sun/Apps/RadioKit/rk-arduino`) as a `lib_dep` so `RadioKitLib.h` and its widget classes are available to `src/`.

#### Scenario: PlatformIO build with RadioKit
- **WHEN** running `pio run -e TRACKLINK_V3`
- **THEN** the build succeeds with the RadioKit library resolved from the local symlink

#### Scenario: RadioKit header is includable
- **WHEN** `src/RADIOKIT.h` includes `<RadioKitLib.h>`
- **THEN** the widget and library symbols (`RK_Knob`, `RK_GasPedal`, `RK_MultipleSelect`, `RK_Slider`, `RK_SlideSwitch`, `RK_PushButton`, `RK_Telemetry`, `RadioKit`) are available

### Requirement: RC_UI widget declarations
The firmware SHALL declare the widgets from the saved RC_UI design: page 0 "Truck" (steering_wheel knob, gas_pedal, brake_pedal, led_select) and page 1 "Loco" (slider, dir_switch, lights_toggle, horn), plus Battery and Speed telemetry widgets.

#### Scenario: Truck page widgets exist
- **WHEN** the firmware boots and RadioKit initializes
- **THEN** `steering_wheel`, `gas_pedal`, `brake_pedal`, and `led_select` are registered on page 0

#### Scenario: Loco page widgets exist
- **WHEN** the firmware boots and RadioKit initializes
- **THEN** `slider`, `dir_switch`, `lights_toggle`, and `horn` are registered on page 1

### Requirement: RadioKit lifecycle
The firmware SHALL initialize RadioKit widgets and pages via `initRadioKit()`, resolve `RadioKit.config.name`, `RadioKit.config.description`, and `RadioKit.config.type` via `applyDeviceMetadata()`, and explicitly commit configuration and start transports in `src/main.cpp` using the following staged sequence:
1. `initRadioKit()`: Configure widget definitions, page names, and orientations without starting transports or clobbering dynamic metadata.
2. `applyDeviceMetadata()`: Resolve `RadioKit.config.name` and `RadioKit.config.description` using the 3-tier cascade:
   - Hardware config top-level `name` and `description` (if present and non-empty).
   - Vehicle config `name` and `description` (`vehicle.name` and `vehicle.description`, if present and non-empty).
   - Fallback defaults (`"RC_UI"` and `""`).
   Set `RadioKit.config.type` to `"Locomotive"` for LOCOMOTIVE and `"Truck"` for truck-family types, setting active page accordingly.
3. `RadioKit.begin()`: Commit the resolved dynamic configuration.
4. `RadioKit.startSerial(Serial)` & `RadioKit.startBLE()`: Start transports broadcasting the resolved name.
5. `RadioKit.enableFS()`: Mount LittleFS for the RadioKit filesystem protocol.

On config hot-reload (`reloadConfigs()`), the firmware SHALL re-evaluate the metadata cascade and update `RadioKit.config.name`, `RadioKit.config.description`, `RadioKit.config.type`, and the active BLE advertising name.

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

### Requirement: Telemetry widget registration
The firmware SHALL register `telemetry_Battery` (unit "%") and `telemetry_Speed` (unit "km/h") output widgets, and continuously update their string buffers during operation at a 250ms interval.

#### Scenario: Telemetry visible in app
- **WHEN** the app displays the device
- **THEN** Battery and Speed telemetry readouts are present with their initial "--" content

#### Scenario: Real-time telemetry streaming
- **WHEN** the vehicle controller runs its telemetry update cycle
- **THEN** `telemetry_Battery` is formatted as integer percentage (0–100) and `telemetry_Speed` is formatted as km/h (0–200, mapped as `abs(motorSpeed) * 2`)
