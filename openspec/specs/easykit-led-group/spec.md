# easykit-led-group Specification

## Purpose

Defines how the RC_brain firmware wires the `EasyLEDGroup` class from the ESP32_EasyKit library (sibling repo `../ESP32_EasyKit`) into the vehicle: ditch lights driven by a two-member group, and the main-loop pump that advances every light instance. The `EasyLEDGroup` class API itself is specified in the library's own OpenSpec repository.

## Requirements

### Requirement: Ditch lights use the group

The locomotive ditch light (two outputs flashing alternately) SHALL be
driven by a two-member `EasyLEDGroup` with the `alternate` pattern instead of
manual alternation state in `VehicleController`. The hardware config keys are
unchanged (`left`/`right`/`brightness_max`/`interval_ms`), and the app toggle
(loco light selector item F, bit 5) SHALL start and stop the pattern.

#### Scenario: Ditch toggle off stops the alternation

- **WHEN** the ditch app toggle is turned off
- **THEN** both ditch outputs are off and no further alternation occurs

### Requirement: All LED instances are pumped

`HardwareInit::update()` SHALL advance the animation engine of every light
instance — including `ditchLLed`, `ditchRLed`, `stepLed`, `cabLed` — and the
ditch group, so engine-driven animations on any light actually progress.

#### Scenario: Aux light engine ticks

- **WHEN** an animation (fade/blink/group pattern) is started on a light not
  previously in the pump list
- **THEN** it progresses each main-loop iteration
