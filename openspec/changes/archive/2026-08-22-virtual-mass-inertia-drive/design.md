## Context

In `RC_brain`, `VehicleController::update()` runs in the main Arduino loop (~50–100 Hz) and receives normalized input from the RadioKit UI (`gas_pedal`, `brake_pedal`, `gear_switch`). Currently, `motorSpeed` is computed as:
```cpp
int16_t motorThrottle = throttlePct;
if (parkingBrake) {
    motorThrottle = 0;
} else if (brakePct > 20) {
    motorThrottle = (int16_t)((int32_t)throttlePct * (100 - brakePct) / 80);
}
int16_t motorSpeed = reverse ? -motorThrottle : motorThrottle;
```
This directly applies the target throttle to the hardware driver without taking into account physical vehicle weight, resulting in instantaneous acceleration and abrupt stopping.

## Goals / Non-Goals

**Goals:**
- Implement a time-based virtual mass inertia filter in `VehicleController` for motor drive output.
- Support configurable acceleration (`acc`), coasting deceleration (`dec`), and active braking deceleration rates.
- Provide a robust fallback to 1:1 direct mode when no vehicle profile exists, when `/vehicle-config.json` lacks an `"engine"` block, or when inertia is explicitly set to 0.
- Synchronize air-brake sound triggers upon vehicle coming to a complete stop from inertia coasting.
- Preserve full compatibility with both Ackermann steering and Skid-Steer drivetrains.

**Non-Goals:**
- Modifying the underlying audio sampling rate or DSP mixer core.
- Replacing hardware PWM frequencies.

## Architecture & Data Flow

```
┌─────────────────┐       ┌────────────────────────┐
│  RadioKit Input │       │ /vehicle-config.json   │
│  (gas / brake)  │       │ (engine.acc, dec, etc) │
└────────┬────────┘       └───────────┬────────────┘
         │                            │
         ▼                            ▼
┌───────────────────────────────────────────────────┐
│              VehicleController::update            │
├───────────────────────────────────────────────────┤
│ 1. Evaluate Direct Mode Condition:                │
│    • s_profile == nullptr                         │
│    • !s_profile->hasEngineConfig                  │
│    • cfg.engine.inertia == 0                      │
│                                                   │
│ 2. Inertia Drive State Machine:                   │
│    IF Direct Mode:                                │
│       s_rampedSpeed = targetSpeed;                │
│    ELSE:                                          │
│       • Acceleration: ramp towards targetSpeed    │
│       • Coasting: decay towards 0                 │
│       • Braking: rapid decay with brake scale     │
│                                                   │
│ 3. Interlocks:                                    │
│    • Gear == Park OR Engine == OFF => speed = 0   │
└─────────────────────────┬─────────────────────────┘
                          │
          ┌───────────────┴───────────────┐
          ▼                               ▼
┌──────────────────┐            ┌───────────────────┐
│ Ackermann Motor  │            │ Skid-Steer Motors │
│ setMotor(speed)  │            │ setSkid(L, R)     │
└──────────────────┘            └───────────────────┘
```

## Decisions

### Decision 1: Inertia Ramp Location (VehicleController vs RcEngineSound)
- **Choice**: Implement the physical motor drive ramping in `VehicleController`.
- **Rationale**: `RcEngineSound` is responsible for sound generation and audio state. Placing the physical motor PWM ramping in `VehicleController` keeps audio synthesis and physical actuator control decoupled while sharing configuration parameters.

### Decision 2: Direct Mode Fallback Detection
- **Choice**: Inspect `s_profile` metadata: if `s_profile == nullptr`, `!s_profile->config.hasEngine`, or `cfg.engine.inertia == 0` (or `acc == 0 && dec == 0`), pass `targetSpeed` directly to `HardwareInit::setMotor()`.
- **Rationale**: Ensures standard RC models, simple crawlers, and unconfigured boards retain instant 1:1 control with 0 latency.

### Decision 3: Millisecond-Delta Time-Step Ramping
- **Choice**: Use elapsed millisecond deltas (`now - s_lastInertiaTime`) with fixed-point math rather than assuming a fixed loop frequency.
- **Rationale**: Guarantees consistent physics and acceleration ramp rates regardless of loop jitter or Wi-Fi/BLE communication load.

## Risks / Trade-offs

- **[Risk] Slower emergency stop reaction in high-inertia heavy vehicles** → *Mitigation*: Hard brake pedal (`brakePct > 80`) or switching gear to `Park` applies an emergency override that rapidly clamps motor speed to 0.
- **[Risk] Skid-steer track creep when stopped with steering applied** → *Mitigation*: Enforce zero speed on both tracks when linear target is zero and below the stopped threshold.
