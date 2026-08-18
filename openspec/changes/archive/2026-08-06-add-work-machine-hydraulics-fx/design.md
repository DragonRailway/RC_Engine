## Context

The `RcEngineSound` synthesis engine includes sound trigger slots and audio synthesis routines for `HYDRAULIC_PUMP`, `HYDRAULIC_FLOW`, `TRACK_RATTLE`, `BUCKET_RATTLE`, and `DUMP_BED`. However, these audio triggers and physical actuator outputs are not yet evaluated in `VehicleController::update()` or exposed via `HardwareInit`.

## Goals / Non-Goals

**Goals:**
- Evaluate auxiliary hydraulic inputs (`aux_hydraulic1`, `aux_hydraulic2`) in `VehicleController::update()`.
- Automatically trigger `HYDRAULIC_FLOW` sound hiss when auxiliary hydraulic inputs exceed ±10%.
- Implement engine pump load governor simulation: bumping target engine RPM by +20% during active hydraulic flow.
- Evaluate track pin rattle (`TRACK_RATTLE`) when `features.trackRattleEnabled` is true and `virtualSpeed > 0`.
- Implement auxiliary physical servo outputs (`auxServo1`, `auxServo2`) in `HardwareInit` mapped to pins Servo 2 and Servo 3 via `EasyServo`.
- Expose clear control variables (`aux_hydraulic1`, `aux_hydraulic2`, `bucket_rattle_trigger`, `dump_bed_toggle`) for manual RadioKit UI widget binding.

**Non-Goals:**
- Automatic RadioKit UI layout generation (user handles UI layout manually).
- Shaker motor vibration output.

## Decisions

### 1. Engine Pump Load Governor Simulation
- **Decision**: When `aux_hydraulic1` or `aux_hydraulic2` magnitude exceeds 10%, `VehicleController` sets `s_hydraulicFlowActive = true`.
  - Sound engine trigger: `s_engine->triggerHydraulicFlow(true)`
  - RPM calculation: `int16_t rpmTarget = baseRpm + (s_hydraulicFlowActive ? (s_profile->config.engine.maxRpm * 20 / 100) : 0);`
- **Rationale**: Real diesel excavators and construction machinery automatically rev up the engine governor when hydraulic valves open to supply oil pressure to heavy boom/arm cylinders.

### 2. Physical Auxiliary Servo Channels
- **Decision**: Extend `HardwareInit` to declare `static EasyServo auxServo1;` (pin S1/Servo 2) and `static EasyServo auxServo2;` (pin S2/Servo 3).
  - Map `aux_hydraulic1` (-100..+100) to `auxServo1` (1000us..2000us).
  - Map `aux_hydraulic2` (-100..+100) to `auxServo2` (1000us..2000us).
- **Rationale**: Allows direct physical control over dump bed lift servos, excavator boom/arm servos, or winch servos.

## Risks / Trade-offs

- **[Risk] High RPM Overflow when Combining Throttle + Hydraulics**:
  - *Mitigation*: Clamp total target RPM in `VehicleController` to `s_profile->config.engine.maxRpm`.
