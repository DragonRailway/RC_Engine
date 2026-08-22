## Context

In `RcEngineSound.cpp`, automatic transmission (`TRANS_AUTOMATIC`) calculates gear shift points and gear targets. However, without torque converter slip, the engine RPM tracks wheel speed directly in gear without the characteristic flaring of an Allison / heavy automatic torque converter when accelerating under load.

The reference project `Rc_Engine_Sound_ESP32` computes engine load and adds a load-dependent `converterSlip` offset to the base gear-ratio target RPM.

## Goals / Non-Goals

**Goals:**
- Implement engine load tracking (`engineLoad = throttle - currentRpm`).
- Implement torque converter slip calculation (`converterSlip = engineLoad * torqueConverterSlip / 100`) with 2x launch multiplier in 1st and Reverse gear.
- Provide configurable `torque_converter_slip` in JSON config (default: 100%).
- Ensure clean transitions into steady-state lockup when cruising at speed.

**Non-Goals:**
- Physical hydraulic line pressure or temperature modeling.
- Altering manual transmission or direct drivetrain behavior.

## Decisions

1. **Engine Load Bounding**:
   - Engine load is defined as `throttle - currentRpm`, bounded to `[0, 180]`.
   - If throttle is less than current RPM (coasting or deceleration), `engineLoad = 0` so no artificial slip flare occurs on overrun.

2. **Launch Multiplication**:
   - In 1st gear (`selectedGear == 0`) or Reverse (`selectedGear == 0` with reverse direction), slip is multiplied by 2 to simulate the high stall torque ratio of a torque converter from dead stop.

3. **Effective Target RPM Composition**:
   - `effectiveTarget = gearBaseRpm + throttleInGear + converterSlip;`
   - Clamped to `cfg.engine.maxRpm`.

## Risks / Trade-offs

- *[Risk]*: Over-flaring causing engine to sit permanently at redline during acceleration.
  - *Mitigation*: Bounding `engineLoad` to 180 and `effectiveTarget` to `maxRpm`, with natural decay as `currentRpm` rises.
