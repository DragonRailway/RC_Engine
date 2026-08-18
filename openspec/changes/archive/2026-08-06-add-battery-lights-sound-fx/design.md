## Context

The current `RC_brain` codebase integrates RadioKit BLE/Serial control, `ESP32_EasyKit` motor/servo/LED drivers, and the `RcEngineSound` synthesis engine. However, several critical real-world features are missing:
1. **Battery Protection**: Voltage reading was only converted to telemetry percentage without LiPo cell count detection or motor cutoff on low voltage.
2. **Lighting Automation**: Headlights and turn signals were simple manual toggles without steering wheel auto-cancellation or deceleration brake light triggering.
3. **Sound FX & Engine State**: The engine booted directly into `RUNNING` without a startup sequence, and physics-driven sound effects (Jake brake, Turbo wastegate) lacked automatic evaluation loops.

## Goals / Non-Goals

**Goals:**
- Implement auto cell count detection at boot for 2S (7.4V nominal), 3S (11.1V nominal), and 4S (14.8V nominal) LiPo packs.
- Implement low-voltage safety cutoff (3.3V/cell): disabling motor drive, flashing hazard lights, and playing `outOfFuel` audio alert.
- Automate turn signal cancellation based on steering position (< ±10% turns off indicators active from > ±35% turn).
- Enable dynamic brake light triggering on rapid throttle drop.
- Implement 3-state headlight stepping (Off, Low Beam 40%, High Beam 100%).
- Provide a manual Engine Start/Stop button in RadioKit UI, transitioning through cranking sound sequence.
- Physics-based auto-triggers for Jake Brake (high RPM deceleration) and Turbo Wastegate blow-off (sudden throttle drop).

**Non-Goals:**
- Physical shaker motor output (deferred).
- Auxiliary work machine hydraulics (excavator/crane controls - deferred).
- Traditional physical RC receiver support (SBUS/IBUS/SUMD - deferred).
- Onboard TFT LCD display driver (telemetry streamed to phone app instead).

## Decisions

### 1. Battery Auto Cell Count Detection
- **Decision**: On system boot, `VehicleController` reads battery voltage across 10 samples.
  - If $V < 8.4\text{V}$, assume 2S pack $\rightarrow$ Cutoff = $6.6\text{V}$.
  - If $8.4\text{V} \le V < 12.6\text{V}$, assume 3S pack $\rightarrow$ Cutoff = $9.9\text{V}$.
  - If $V \ge 12.6\text{V}$, assume 4S pack $\rightarrow$ Cutoff = $13.2\text{V}$.
- **Rationale**: Eliminates hardcoded cell count configuration while preventing permanent LiPo over-discharge damage.

### 2. Auto Turn-Signal State Machine
- **Decision**: Track steering input hysteresis:
  - Steer position > +35% $\rightarrow$ Turn Right indicator ON.
  - Steer position < -35% $\rightarrow$ Turn Left indicator ON.
  - When steering returns to center ($|steer| < 10\%$), deactivate indicator after a short 200ms debounce.

### 3. Engine Start State Machine
- **Decision**: Engine boots in `OFF` state. Drive motor commands are ignored while `OFF`.
- When Engine Start button is pressed in RadioKit:
  1. Trigger engine cranking sound (`STARTING` state).
  2. Upon cranking sample completion, transition state to `RUNNING` and start idle loop sound.
  3. Allow drive motor operation.

## Risks / Trade-offs

- **[Risk] Low Voltage False Positive on Heavy Acceleration**: High current draw during sudden motor acceleration can cause temporary voltage sag below cutoff.
  - *Mitigation*: Require low voltage condition to persist continuously for at least 1.5 seconds before triggering motor cutoff.

- **[Risk] Steering Auto-Cancel Interfering with Manual Turn Signals**: User may manually toggle turn signal widget while steering.
  - *Mitigation*: Manual turn signal UI toggle overrides auto-steering turn signal mode.
