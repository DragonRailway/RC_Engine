## Why

Currently, `HardwareInit` statically allocates only 2 `EasyMotor` driver instances (`driveMotor` and `auxMotor`) and a limited set of `EasyServo` instances. Complex multi-actuator RC models and multi-channel boards (like G-Track with up to 4 onboard H-bridges and 8 servo ports) require the ability to run up to 4 physical H-bridge drivers (for drive motors, aux machines, or trailer outputs), 4 physical servo/ESC outputs, and up to 10 LEDs concurrently.

This change expands the physical hardware object pools in `HardwareInit` to support 4 independent H-bridge motor drivers (`DRIVER_A` through `DRIVER_D`), 4 physical servo/ESC outputs, and ensures all 10 physical LEDs are seamlessly addressed from JSON configurations.

## What Changes

- **Physical Driver Pool Expansion (Up to 4 Drivers)**: Expand `HardwareInit` to support up to 4 physical `EasyMotor` instances (`DRIVER_A`, `DRIVER_B`, `DRIVER_C`, `DRIVER_D`), assignable to drive motors, aux motors, or trailer outputs.
- **Physical Servo Pool Expansion (Up to 4 Servos)**: Expand `HardwareInit` to support up to 4 physical `EasyServo` instances claimable across steering servos, aux servos, and ESC drive motors.
- **PinMapper & Constants**: Add `DRIVER_C = 0xE3` and `DRIVER_D = 0xE4` hardware markers to `PinMapper.h`.
- **JSON Schema & Validation**: Ensure `hardware_config.schema.json` and `validate_configs.py` allow `DRIVER_A`..`DRIVER_D` across all motor/aux bindings.
- **Preserve Logical Vehicle Limits**: Maintain existing high-level vehicle physics interfaces (2 drive channels, 2 steering servos, 2 aux motors) while routing their physical outputs through the 4-driver / 4-servo pool.

## Capabilities

### New Capabilities
- `hardware-driver-servo-expansion`: Physical object pool managing up to 4 H-bridge drivers (`DRIVER_A`..`DRIVER_D`) and up to 4 concurrent servo/ESC channels.

### Modified Capabilities
*(None. Existing vehicle physics, lighting logic, and runtime configuration APIs remain 100% backward compatible.)*

## Impact

- **`common/HardwareInit.h` / `HardwareInit.cpp`**: Replace single `driveMotor`/`auxMotor` instances with `EasyMotor s_motorDrivers[4]` pool and `EasyServo s_servos[4]` pool.
- **`common/PinMapper.h`**: Add `DRIVER_C` and `DRIVER_D` definitions and resolution.
- **`configs/schemas/hardware_config.schema.json`**: Update schema definitions.
- **`test/host_vc/`**: Verify test suite passes with multi-driver/servo routing.
