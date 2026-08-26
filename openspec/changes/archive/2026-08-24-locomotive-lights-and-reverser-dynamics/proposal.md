## Why

The reference `TrainControl_BLE` controller features realistic incandescent lamp fading, soft ditch light cross-fading, and safe direction reversal. Incorporating these behaviors into the RadioKit locomotive profile will provide scale model realism, automatic directional lighting handover, and prototypical train momentum interlocking when switching the reverser.

## What Changes

- **Automatic Directional Lighting**: When driving forward (`dir_switch` = 1), the forward Headlight is energized and Rear Marker/Tail is dimmed or turned off; in reverse (`dir_switch` = 0), the Rear Marker/Tail light is energized and Headlight is dimmed or turned off.
- **Incandescent Bulb Soft PWM Fade**: Implements asymmetric smooth fade-in and quick fade-out transitions on all locomotive lighting channels (Headlight, Cab, Step/Ground, Tail/Marker) to simulate prototypical incandescent filament warming and cooling.
- **Ditch Light Soft Cross-Fade**: Upgrades locomotive ditch lights (`L4`, `L5`) to use smooth triangular/sinusoidal PWM cross-fading between left and right fixtures, active manually or automatically triggered during bell/horn sequences.
- **Reverser Momentum Interlock & Throttle Auto-Zero**: Zeroes throttle command when the reverser switch (`dir_switch`) is flipped while in motion, decelerating smoothly via kinetic momentum and dynamic braking before engaging the opposite direction.

## Capabilities

### New Capabilities
- `locomotive-directional-lighting`: Automatic reverser-coupled directional handover for locomotive headlight and rear marker channels.
- `locomotive-lighting-effects`: Incandescent bulb asymmetric PWM fade simulation and soft triangular ditch light cross-fading.
- `locomotive-reverser-dynamics`: Directional change safety interlock, kinetic deceleration, and throttle zeroing on reverser flip.

### Modified Capabilities
<!-- No requirement changes to existing main specs -->

## Impact

- Firmware vehicle control loops in `RC_brain/common/VehicleController.h` and `RC_brain/src/main.cpp`.
- Hardware configuration parameters in `RC_brain/configs/hardware_configs/hardware-TRACKLINK_V3-locomotive.json`.
- App UI interaction handling for locomotive controls and telemetry feedback.
