## Context

RC vehicles and model locomotives often feature multiple hardware outputs that operate under a single control channel:
- Dual powered bogies (front and rear trucks on locomotives like TrackLink V3 using `DRIVER_A` and `DRIVER_B`).
- Dual steering servos (4-wheel or split-linkage steering using `S1` and `S2`).
- Multi-LED lighting channels (e.g. twin headlights or multiple marker lights on `["L1", "L2"]`).
- Multi-channel aux systems (dual winches, dual hydraulic pumps).

The previous architecture assumed single instances (`drive_motor`, `steering_servo`, single pin `hardware: "L1"`, single `aux_motor`), forcing single-driver operation or requiring custom firmware workarounds.

## Goals / Non-Goals

**Goals:**
- Provide clean, uniform array-based JSON configuration for multi-instance hardware across all peripherals (`drive_motors`, `steering_servos`, multi-pin `hardware` in lights, and `aux_motors`).
- Support dual-bogie locomotive drive synchronization where each motor independently configures its polarity (`forward`/`reverse`), frequency, and duty window.
- Designate the primary driver (`drive_motors[0]`) for back-EMF (BEMF) feedback sensing.
- Implement zero-legacy, clean C++ structs with fixed-size bounds (`MAX_DRIVE_MOTORS=2`, `MAX_STEERING_SERVOS=2`, `MAX_AUX_MOTORS=2`, `MAX_PINS_PER_LIGHT=4`).
- Update the schema validation (`configs/schemas/hardware_config.schema.json`) and all repository hardware configs.

**Non-Goals:**
- Backward compatibility with legacy singular keys or uppercase JSON tokens (deliberately omitted for architectural cleanliness).
- Differential traction control or torque vectoring across drive bogies (both bogies track the unified vehicle drive speed command).

## Decisions

### Decision 1: Canonical Plural Arrays in JSON
Instead of polymorphic type checks (`is<JsonArray>` vs `is<JsonObject>`) or ad-hoc names (`drive_motor_2`, `front_motor`), all multi-instance hardware will use canonical plural array keys:
- `drivetrain.drive_motors`: `[ { "hardware": "DRIVER_A", ... }, { "hardware": "DRIVER_B", ... } ]`
- `drivetrain.steering_servos`: `[ { "hardware": "S1", ... } ]`
- `lights.<name>.hardware`: `[ "L1", "L2" ]`
- `aux_motors`: `[ { "hardware": "DRIVER_B", ... } ]`

*Alternative considered*: Polymorphic parser accepting single object or array. Rejected because backward compatibility is not needed and strict arrays ensure cleaner schemas and simpler parsing logic.

### Decision 2: Fixed-Size Static Structs in Firmware
In `Config.h`, multi-instance channels are stored as fixed-size inline arrays with count trackers:
```cpp
static constexpr uint8_t MAX_DRIVE_MOTORS    = 2;
static constexpr uint8_t MAX_STEERING_SERVOS = 2;
static constexpr uint8_t MAX_AUX_MOTORS       = 2;
static constexpr uint8_t MAX_PINS_PER_LIGHT  = 4;

DriveMotor    driveMotors[MAX_DRIVE_MOTORS];
uint8_t       driveMotorCount = 0;

SteeringServo steeringServos[MAX_STEERING_SERVOS];
uint8_t       steeringServoCount = 0;
```
*Rationale*: Predictable, zero-heap runtime memory on ESP32-S3 without dynamic allocation fragmentation.

### Decision 3: Synchronized Channel Control & Primary BEMF
In `HardwareInit.h`:
- `setMotor(speed)` iterates across all active channels in `s_driveCh[0..driveMotorCount-1]`.
- Each motor channel handles its own directional polarity (`FORWARD` vs `REVERSE`) and duty scaling (`dutyMin..dutyMax`).
- Primary motor (`s_driveCh[0]`) provides BEMF speed feedback for the load governor.
- `setServo(pos)` iterates across `s_steeringServos[0..steeringServoCount-1]`, mapping position independently against each servo's endpoints.
- `setLight(pin, brightness)` and animations control all pins assigned to a light channel in lockstep.

## Risks / Trade-offs

- [Risk: Simultaneous high stall current with dual motors] → Mitigation: Each motor channel is connected to separate H-bridge ICs on the board (e.g. DRIVER_A and DRIVER_B on TrackLink V3), distributing thermal and electrical load across independent circuits.
- [Risk: Misaligned bogie directions causing physical gear binding] → Mitigation: Explicit `direction: "reverse"` in the hardware JSON allows immediate configuration alignment without physical rewiring.
- [Risk: Existing hardware configs failing validation] → Mitigation: Update all 5 hardware configuration files in `configs/hardware_configs/` as part of the implementation.
