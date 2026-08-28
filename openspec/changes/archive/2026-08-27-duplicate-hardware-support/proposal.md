## Why

Vehicles with multiple powered bogies (such as dual-truck locomotives on TrackLink V3), multi-axle steering vehicles (such as 4-wheel steering trucks), multi-LED lighting circuits (such as paired headlights or cab lights), and multi-actuator aux systems need to drive multiple physical outputs from a single logical subsystem. Currently, the hardware configuration and firmware only allow single instances for `drive_motor`, `steering_servo`, single-pin light assignments, and single `aux_motor`, preventing dual-driver locomotive operation and multi-pin output configurations.

## What Changes

- **BREAKING**: Replaced single-instance hardware configuration keys with canonical plural array schemas:
  - `drivetrain.drive_motors`: Array of 1 to 2 motor objects (supporting independent direction, frequency, and duty windows per motor/bogie).
  - `drivetrain.steering_servos`: Array of 1 to 2 servo objects (supporting independent endpoints and frequencies for multi-axle steering).
  - `lights.<light_name>.hardware`: Array of 1 to 4 pin name strings (e.g. `["L1", "L2"]`), binding multiple physical LEDs to a single logical light channel.
  - `aux_motors`: Array of 1 to 2 aux motor objects.
- **Drivetrain Multi-Driver Execution**:
  - `HardwareInit::setMotor(speed)` commands all configured drive motor channels in sync, applying individual direction and min/max duty configurations.
  - Primary motor (`drive_motors[0]`) is designated for BEMF speed sensing.
- **Multi-Servo & Multi-LED Execution**:
  - `HardwareInit::setServo(pos)` commands all configured steering servos in sync according to their respective endpoint ranges.
  - `HardwareInit::setLight(pin, brightness)` and animation engines drive all bound pins simultaneously.
- **Schema & Validation**:
  - Updated `configs/schemas/hardware_config.schema.json` to enforce the new plural array structures.
  - Updated all hardware config files in `configs/hardware_configs/` to the canonical schema.

## Capabilities

### New Capabilities
- `multi-hardware-peripherals`: Canonical schema and runtime support for multi-instance drive motors, steering servos, multi-pin LED channels, and aux motors.

### Modified Capabilities
- `mandatory-drivetrain-config`: Drivetrain schema now requires `drive_motors` array instead of single `drive_motor` object.
- `aux-outputs`: Aux motor schema now uses `aux_motors` array.

## Impact

- **Firmware (`common/`)**:
  - `Config.h`: Structs updated with fixed-size arrays (`MAX_DRIVE_MOTORS=2`, `MAX_STEERING_SERVOS=2`, `MAX_AUX_MOTORS=2`, `MAX_PINS_PER_LIGHT=4`).
  - `ConfigParser.h`: Simplified parser using uniform array iteration, removing single-object vs array branching and legacy uppercase fallback keys.
  - `HardwareInit.h`: Motor/servo channel arrays and multi-pin LED group dispatching.
  - `VehicleController.h`: Safety paths (`setAllMotors`) stop all configured channels.
- **Configs & Schemas**:
  - `configs/schemas/hardware_config.schema.json` updated with strict array validations.
  - `configs/hardware_configs/*.json` updated to match the new schema.
