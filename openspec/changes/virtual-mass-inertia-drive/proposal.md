## Why

Currently, physical motor output (`motorSpeed`) in `VehicleController` directly follows the UI `gas_pedal` position without applying vehicle mass inertia. While the audio engine simulates engine rotational flywheel dynamics, heavy trucks and locomotives instantly jump and stop physically. Implementing a virtual mass inertia layer replicates authentic scale driving physics (gradual acceleration, gear-dependent ramping, coasting, and realistic braking deceleration) while providing an automatic fallback to direct 1:1 control when no vehicle config or engine section is defined.

## What Changes

- **Virtual Mass Inertia Drivetrain Ramping**: Add a physics-based motor speed filter in `VehicleController` that ramps physical motor speed according to configurable acceleration rates (`accel_rate`), coasting/deceleration rates (`decel_rate`), and braking deceleration (`brake_rate`).
- **Direct Mode Fallback**: Automatically bypass inertia simulation and operate in 1:1 direct throttle mode if `/vehicle-config.json` is missing, invalid, or does not contain an `"engine"` configuration section.
- **Gear-Dependent Inertia Scaling**: Scale motor ramp intervals based on the active gear (e.g. higher inertia resistance in higher gears) when a transmission profile is defined.
- **Coasting & Air Brake Integration**: Smooth deceleration when throttle is released, and trigger air brake release sound on coming to a complete stop from motion.
- **Skid-Steer & Ackermann Support**: Apply the ramped speed cleanly across both Ackermann drive motors and differential skid-steer tracks without steering jitter.

## Capabilities

### New Capabilities
- `virtual-mass-inertia`: Physics-based vehicle mass and drivetrain inertia ramping for physical motor outputs with automatic direct-mode fallback.

### Modified Capabilities
<!-- None -->

## Impact

- `common/VehicleController.h`: Motor drive path in `update()` filtered through the inertia state machine.
- `lib/SoundEngine/src/RcEngineSound.h`: Exposes vehicle mass simulation configuration or helper methods if shared.
- `common/Config.h`: Struct definitions updated with inertia drive parameters if needed (`esc_ramp_time`, `esc_accel_steps`, `esc_brake_steps`).
- Zero breaking changes to existing REST/BLE APIs or widgets.
