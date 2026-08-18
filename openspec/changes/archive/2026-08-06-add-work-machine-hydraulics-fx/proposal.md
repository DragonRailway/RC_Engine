## Why

Work machines such as excavators, dozers, and dump trucks require realistic hydraulic sound synthesis, track pin clanking, and physical auxiliary servo outputs (boom, arm, bucket, dump bed). Adding support for hydraulic flow hiss, engine pump load governor simulation (+20% RPM bump), speed-dependent track rattle, and physical auxiliary servo channels will expand `RC_brain` to fully support construction and heavy utility vehicles.

## What Changes

- **Hydraulic Pump & Flow Synthesis**: Enable continuous background hydraulic pump hum (`hydraulicEnabled`) and trigger hydraulic flow hiss when auxiliary hydraulic inputs exceed 10%.
- **Engine Hydraulic Governor Simulation**: Automatically bump simulated engine idle RPM by +20% during active hydraulic flow to simulate pump load.
- **Track Rattle FX**: Implement automatic rhythmic pin clanking for tracked vehicles when moving, with interval scaling dynamically from 500ms down to 90ms based on speed.
- **Physical Auxiliary Servo Channels**: Expand `HardwareInit` and `HardwareConfig` to drive physical auxiliary servos (Aux Servo 1 / Servo 2 pin, Aux Servo 2 / Servo 3 pin) using `EasyServo`.
- **Modular Control API**: Expose clear control variables (`aux_hydraulic1`, `aux_hydraulic2`, `bucket_rattle_trigger`, `dump_bed_toggle`) in `VehicleController` for manual UI widget mapping.

## Capabilities

### New Capabilities
- `work-machine-hydraulics`: Hydraulic flow synthesis, engine load governor simulation, track pin rattle, and physical auxiliary servo channel control.

### Modified Capabilities
- `vehicle-control-loop`: Update vehicle loop to evaluate auxiliary hydraulic inputs, hydraulic RPM load governor, track rattle intervals, and auxiliary servo positioning.

## Impact

- **Affected Code**: `common/VehicleController.h`, `common/HardwareInit.h`, `common/Config.h`, `lib/SoundEngine/src/RcEngineSound.h`.
- **Dependencies**: No external library additions; uses existing `ESP32_EasyKit` (`EasyServo`) and `RcEngineSound`.
