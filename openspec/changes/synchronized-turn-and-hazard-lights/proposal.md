## Why

When a turn indicator is active and the user switches on hazard lights, the left and right LEDs flash out of phase because their blink timers are initialized independently. Introducing a first-class synchronized turn and hazard subsystem ensures zero phase drift and aligns LED flashes with the cabin flasher audio click.

## What Changes

- **Synchronous Turn & Hazard Controller**: Replace independent pin-level `setLightBlink` calls with `HardwareInit::setTurnSignals(left, right, hazard, intervalOn, intervalOff, brightness)`.
- **Phase-Locked Hazard Transitions**: When hazard mode is engaged from any state (OFF, Left, or Right), both left and right blink timers are reset simultaneously in the exact same millisecond.
- **Audio-Visual Flasher Synchronization**: Reset the indicator click sound timing on mode transitions so the cabin audio click triggers in lockstep with the light flashes.

## Capabilities

### New Capabilities
<!-- None -->

### Modified Capabilities
- `vehicle-control-loop`: Adds synchronized turn and hazard light state transitions and audio-visual phase locking.

## Impact

- `common/HardwareInit.h` & `HardwareInit.cpp`: Implementation of `setTurnSignals()` and coordinated blink phase management.
- `common/VehicleController.cpp`: Updated light application logic routing turn and hazard signals through the synchronized controller.
