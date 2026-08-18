## Context

The gap audit showed `brake_pedal` currently only drives the brake sound effect (`triggerBrake`) and brake lamp — the motor command ignores it entirely. Separately, the sound engine's `triggerIndicator` voice (with per-vehicle volume + `indicator.json` assets) is never called, so the automatic turn signals are silent. Both fixes are confined to `common/VehicleController.h`.

## Goals / Non-Goals

**Goals:**
- Brake pedal proportionally reduces motor output above a 20% deadband, for Ackermann and skid-steer.
- Indicator click sound plays whenever a turn indicator (auto or hazard) is active.
- Zero UI/config/sound-asset changes.

**Non-Goals:**
- No motor *regenerative* or reversed braking current — only output scaling.
- No new RadioKit widgets, config schema, or sound assets.

## Decisions

### D1. Linear brake blend on the motor path only
In `VehicleController::update()`, compute a blended throttle for motor output:

```
brakePct = brake_pedal.rk.value            // 0..100 (springs to min)
motorThrottle = throttlePct
if (brakePct > 20)                          // deadband matches the existing brake-sound threshold
    motorThrottle = throttlePct * (100 - brakePct) / 100
```

- Ackermann: `setMotor(reverse ? -motorThrottle : motorThrottle)`.
- Skid-steer: apply `motorThrottle` before the differential mix on both sides.
- The sound engine keeps receiving RPM from the **raw** `throttlePct`, so braking doesn't stall the simulation; the existing `triggerBrake` sound and brake-lamp logic are untouched.
- Battery-cutoff path is unaffected (it already forces motor 0 before the blend is computed).

*Alternatives considered*: full stop on any brake press (binary) — rejected, the spec requires proportionality. Applying brake to the RPM/sound input — rejected, keeps the engine simulation independent per the spec scenario.

### D2. Indicator sound follows the effective turn-signal state
Effective indicator state = `hazardActive || s_autoTurnLeft || s_autoTurnRight`. Call `s_engine->triggerIndicator(active)` with change detection via a new `s_indicatorPrev` static (same pattern as `s_hornPrev`). The engine keeps the voice playing while active, so no per-flash sync is needed.

*Alternatives considered*: syncing the sound to the 333 ms flash phase — unnecessary complexity; the level-triggered voice already loops.

## Risks / Trade-offs

- [Brake blend vs. skid-steer mix] Scaling throttle before mixing keeps both tracks proportional → mitigation: blend is applied before the differential formula.
- [Deadband mismatch] Sound triggers at >20 and braking begins at >20, so sound and motion agree.
- [Indicator voice missing in some profiles] If a profile has no `indicator.json`, `triggerIndicator` simply plays nothing (slot empty) — same fallback as all other voices.

## Migration Plan

None — firmware-only, no config or UI changes.

## Open Questions

None.
