# RadioKit BLE Control

## ADDED Requirements

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
The firmware SHALL call `initRadioKit()` during setup (configuring name "RC_UI", type "Locomotive", and starting BLE and Serial transports plus the filesystem feature) and SHALL call `RadioKit.update()` on every loop iteration.

#### Scenario: App connects over BLE
- **WHEN** the RadioKit app connects to the device over BLE
- **THEN** the app receives the device name "RC_UI" and the declared widgets with their states

#### Scenario: Loop keeps RadioKit alive
- **WHEN** the firmware loop runs continuously
- **THEN** `RadioKit.update()` is invoked each iteration so widget state changes propagate

### Requirement: Telemetry widget registration
The firmware SHALL register `telemetry_Battery` (unit "%") and `telemetry_Speed` output widgets.

#### Scenario: Telemetry visible in app
- **WHEN** the app displays the device
- **THEN** Battery and Speed telemetry readouts are present with their initial "--" content
