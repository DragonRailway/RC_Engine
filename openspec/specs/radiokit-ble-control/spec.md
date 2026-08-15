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
The firmware SHALL call `initRadioKit()` during setup (configuring name "RC_UI" and starting BLE and Serial transports plus the filesystem feature) and SHALL call `RadioKit.update()` on every loop iteration. After initialization, the firmware SHALL set the RadioKit device type string from the vehicle config type (`"Truck"` for TRUCK, `"Locomotive"` for LOCOMOTIVE, and the truck string for EXCAVATOR/UNKNOWN) rather than a hardcoded value, and SHALL force the active page to match the vehicle type (page 0 "Truck" for truck-family types, page 1 "Loco" for LOCOMOTIVE) so the app lands on the correct page on connect. The app-initiated page switch remains available to the user afterwards.

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

### Requirement: Telemetry widget registration
The firmware SHALL register `telemetry_Battery` (unit "%") and `telemetry_Speed` output widgets.

#### Scenario: Telemetry visible in app
- **WHEN** the app displays the device
- **THEN** Battery and Speed telemetry readouts are present with their initial "--" content
