## Why

In real vehicles, front wheel caster and pneumatic trail generate self-aligning torque that dynamically returns the steering wheel to center as vehicle forward speed increases, while resting with minimal return force when stationary. Currently in RC_Engine, steering inputs remain locked at whatever angle they were turned to unless the user manually drags the virtual steering wheel back to center. 

Adding dynamic auto-centering provides realistic scale vehicle handling where the steering naturally straightens out as the vehicle accelerates forward, automatically uncurling the virtual on-screen steering wheel in the RadioKit mobile app whenever the user is not actively holding or turning the wheel.

## What Changes

- Add an optional `auto_centering` configuration object within the `drivetrain` section of hardware configurations (`hardware-*.json`) with tunable `enabled`, `base_rate`, `speed_rate`, `max_rate`, and `hold_in_reverse` fields.
- Update `HardwareConfig` in `Config.h` and parser logic in `ConfigParser.cpp` with defaults and range validation for `auto_centering`.
- Update `configs/schemas/hardware_config.schema.json` to define and validate the `auto_centering` schema.
- Implement user interaction detection in `VehicleController` to determine when the user is actively touching/dragging the steering control vs when the wheel is released/passive.
- Implement a speed-dependent steering decay physics step in `VehicleController::update()` for both Ackermann (servo) and Skid-Steer (dual-motor differential) modes.
- Synchronize decayed steering values back to the RadioKit app via `steering_wheel.rk.value` updates so the UI wheel visually unwinds to center in real time.

## Capabilities

### New Capabilities
- `dynamic-steering-auto-centering`: Dynamic speed-proportional return-to-center steering physics across Ackermann and Skid-Steer drivetrains, configurable via the hardware config `drivetrain` block with bidirectional RadioKit UI synchronization.

### Modified Capabilities
- `mandatory-drivetrain-config`: Extends the `drivetrain` schema and parser to optionally include and validate the `auto_centering` configuration block.

## Impact

- **Configuration:** `configs/hardware_configs/hardware-*.json` and `configs/schemas/hardware_config.schema.json`.
- **Hardware Config & Parser:** `common/Config.h` and `common/ConfigParser.cpp`.
- **Control Loop & Physics:** `common/VehicleController.h` and `common/VehicleController.cpp`.
- **UI & RadioKit Integration:** Bidirectional knob updates to `steering_wheel.rk.value` and shadow syncing in `VehicleController.cpp`.
- **Validation & Tests:** `scripts/validate_configs.py` and unit test coverage in `test/`.
