# Proposal: Unified Configured Flag Across All HardwareConfig Structs

## Why

Currently, only light output structs (`Light`, `TurnLight`, `DitchLight`, `AuxLight`, `Charging`) contain a `bool configured = false;` status flag. Sub-system structs like `Sound`, `DriveMotor`, `SteeringServo`, `AuxMotor`, `Animation`, `Battery`, and `Power` lack a `configured` flag.

Standardizing `bool configured = false;` across **all** `HardwareConfig` structs creates a uniform C++ configuration contract. It enables any firmware module (`HardwareInit`, `VehicleController`, diagnostics, and telemetry) to query whether a hardware section was explicitly defined in the JSON file.

## What Changes

1. **Config Struct Updates**:
   - Add `bool configured = false;` to `Sound`, `DriveMotor`, `SteeringServo`, `AuxMotor`, `Animation`, `Battery`, and `Power` structs in `common/Config.h`.

2. **Parser Updates**:
   - In `common/ConfigParser.h`, set `config.<section>.configured = true;` when parsing non-null JSON objects for `sound`, `drivetrain` / `drive_motor`, `steering_servo`, `aux_motor`, `animation`, `battery`, and `power`.
   - Update `HardwareConfig` reset/constructor.

3. **Verification & Tests**:
   - Run `python3 scripts/validate_configs.py`.
   - Run `python3 scripts/host_vc_test.py` (all 13 test suites).
   - Build firmware targets (`TRACKLINK_V3` and `MIKRO_V2`).

## Non-goals

- Altering any hardware pin assignment or timing defaults.
