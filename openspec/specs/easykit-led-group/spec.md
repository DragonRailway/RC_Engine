# easykit-led-group Specification

## Purpose
TBD - created by archiving change easykit-led-group. Update Purpose after archive.
## Requirements
### Requirement: Coordinated multi-LED patterns via a step-sequencer group

The EasyKit library SHALL provide an `EasyLEDGroup` class that owns multiple
`EasyLED` members and plays a timed pattern — a table of steps, each a
per-member duty vector (0–100%) plus a duration in ms — on a shared
`millis()` timeline, repeating until stopped. The group SHALL be polled via
an `update()` method (called from the main loop alongside the per-LED
engines) and SHALL provide built-in pattern factories: `alternate`,
`syncFlash`, `chase`, and `doubleStrobe`.

#### Scenario: Alternating pair (ditch lights)
- **WHEN** a group of two LEDs starts the `alternate(8)` pattern
- **THEN** LED A is at full duty for 8 ms while LED B is off, then LED B is
  full for 8 ms while LED A is off, repeating (~60 alternations per second)

#### Scenario: Sync flash
- **WHEN** a group starts `syncFlash(500, 500)`
- **THEN** all members light together for 500 ms, then all are off for
  500 ms, repeating

#### Scenario: Chase
- **WHEN** a three-member group starts `chase(60)`
- **THEN** exactly one member is lit at a time, moving from first to last
  every 60 ms, repeating

### Requirement: Pattern duty ownership and clean stop

While a pattern is running, the group SHALL own its members' duty (no other
light path may write those pins), and `stop()` SHALL land every member at 0
and mark the group idle, so battery cutoff and hot-reload teardown can halt
patterns with a single call.

#### Scenario: Stop lands members off
- **WHEN** `stop()` is called on a running group
- **THEN** all member LEDs are at 0% duty and the group reports not running

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

### Requirement: Existing single-LED blink behavior unchanged
The existing per-LED blink modes (SIMPLE, BURST, HEARTBEAT, CANDLE, MORSE) and the fade/breathing engines SHALL behave exactly as before, and the group SHALL be a separate, coexisting abstraction.

#### Scenario: Turn-signal blinking unaffected
- **WHEN** a turn-signal blink runs on `turnLLed`/`turnRLed` while a group
  pattern runs on the ditch LEDs
- **THEN** both operate independently and correctly

