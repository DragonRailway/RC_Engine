## Context

RC_brain drives all PWM outputs through the vendored `lib/ESP32_EasyKit` (EasyMotor/EasyServo/EasyLED). Today only the instant-write surface is used: `HardwareInit` maps `setMotor/setServo/setLight` onto `write()/writeMicroseconds()`, and the main loop never calls `EasyServo::update()` or `EasyLED::update()` — so the library's non-blocking animation engine (asymmetric sigmoid easing, fade curves, blink state machines) is fully dormant.

`VehicleController` compensates by hand-rolling light timing: turn signals and hazards flash via `(millis() / 333) % 2` (a hardcoded 1.5 Hz), while the hardware config already declares `turn_light.interval_on/interval_off = 500/500` and `brightness_max` — fields that are parsed but never read. Headlight stepping and aux servos snap instantly between states.

Resource budget (ESP32-S3, verified from `soc_caps.h`): 8 LEDC channels (6 used) and 6 MCPWM operators (4 used on TRACKLINK, 5 with ESC). All animation features reuse existing channels — no budget pressure.

## Goals / Non-Goals

**Goals:**
- Activate the EasyKit animation engine with a single `update()` pump driven from the main loop.
- Ease aux-servo motion (dump bed, mixer, excavator arm) via `write(µs, speed, kIn, kOut)`; keep steering instant.
- Fade headlight 3-state stepping instead of snapping.
- Replace hand-rolled turn/hazard flashing with `EasyLED.startBlink`, honoring the config's `interval_on`/`interval_off`/`brightness_max`.
- Add optional `lower_snake_case` hardware-config fields for animation tunables, with code defaults when absent.
- Keep `HardwareInit::setMotor/setServo/setLight` signatures unchanged.

**Non-Goals:**
- No easing for the drive motor or steering servo (instant RC response is intentional).
- No changes to `lib/ESP32_EasyKit` itself.
- No per-output easing/fade tuning (global defaults only; per-output is a future extension).
- No changes to brake-light or reversing-light behavior beyond what the config already drives.

## Decisions

### D1: Where the update pump lives
`HardwareInit` owns all EasyKit objects, so it gets `HardwareInit::update()`, which fans out to the animation engines: `steeringServo.update()`, `escServo.update()`, `auxServo1.update()`, `auxServo2.update()`, and `update()` on each attached `EasyLED`. `src/main.cpp` calls it every loop iteration after `VehicleController::update()`.
- *Alternative considered:* calling `update()` inside `VehicleController::update()`. Rejected — the controller consumes outputs but does not own the hardware objects; the pump belongs beside them in `HardwareInit`.

### D2: Global `animation` config block, per-output later
A single optional `"animation"` section in the hardware config provides the tunables:

```json
"animation": {
  "easing_speed_deg_s": 180,
  "easing_k_in": 0.3,
  "easing_k_out": 0.6,
  "fade_duration_ms": 250
}
```

Defaults when absent: `180 deg/s`, `kIn 0.2 / kOut 0.8` (gentle start, soft finish), `250 ms`. New `HardwareConfig::Animation` struct with these defaults; `ConfigParser` fills from JSON if present. Turn-signal timing/duty are *not* duplicated here — they come from the existing `turn_light` block (`interval_on`, `interval_off`, `brightness_max`).
- *Alternative considered:* per-servo/per-light tuning keys. Rejected for v1 — aux servos aren't even in the config schema today; a global block covers all three features with minimal plumbing.

### D3: Blink ownership model (the core integration rule)
The engine that owns a light owns its duty. Once `startBlink` (or `fadeTo`) is running, **no `setLight` writes may target that pin** or they will fight frame-by-frame.

- Turn-signal/hazard pins are controlled **only** through a new `HardwareInit::setLightBlink(pin, active, onMs, offMs, dutyPct)`: on a rising edge it calls `startBlink`, on a falling edge `stopBlink`. `VehicleController` computes per-side active states (manual + auto + hazard) exactly as today and calls it with the config interval/duty; `applyLightsWithAutomation` stops calling `setLight` on the turn pins.
- Headlight pin: `setLight` writes are replaced by fade-on-change. `VehicleController` tracks the current target; when the 3-state step changes it calls a fade (raw ticks via `getMaxDuty() * pct / 100`, `EASE_IN_OUT` curve, `fade_duration_ms`).
- Tail light derives its level from the headlight's **live** duty (`headLed.getDutyPercent()`) instead of the target, so it tracks the fade naturally with zero extra state. This removes the snap-inconsistency that a target-based tail would show during the headlight ramp.
- Battery-cutoff and `stopAll()` paths call `stopBlink`/`stopFade`/`stop` on all LEDs so a blink can never strand an LED on.

### D4: Aux-servo easing via µs-space `write()`
`HardwareInit::setAuxServo1/2` switch from `writeMicroseconds(us)` to `write((float)us, anim.easingSpeed, anim.kIn, anim.kOut)` when easing speed > 0 (0 disables easing → instant, current behavior). Steering, ESC, and drive motor keep instant writes — `writeMicroseconds` cancels any in-flight move, which is the desired behavior for those channels.
- *Note:* `EasyServo::write` interprets a value ≥ 500 as microseconds, so µs-space easing needs no angle mapping.

### D5: Curve choices
Headlight fades use `EASE_IN_OUT` (sigmoid, k=0.5) — a natural lamp warm-up; `LINEAR` is the fallback if the softer curve feels sluggish. Blink patterns use square-wave `startBlink` (no duty ramping) to keep turn-signal legality/visibility.

## Risks / Trade-offs

- **`write()` vs. fade/blink interleaving** → The D3 ownership model is the contract; a code review gate checks no `setLight` call targets a pin owned by a blink/fade engine.
- **Behavioral change: flash rate 1.5 Hz → 1 Hz** on both shipped configs (config says 500/500). Intended — the config was already authoritative-but-ignored; the spec delta (`advanced-lighting-automation`) records it.
- **Aux-servo response latency** from easing (up to ~move duration) → Speed defaults are modest (180 deg/s); steering/drive unaffected. `easing_speed_deg_s: 0` restores instant.
- **MIKRO_V2's 9 defined LED pins vs. 8 S3 channels** → Pre-existing budget ceiling, unchanged by this work; both shipped configs use 6 lights. Flagged, not fixed here.
- **Stranded blink on state changes** (engine off, cutoff) → Every teardown/stop path calls `stop()` on LEDs; covered in tasks.
- **EasyServo shared `static lastUpdate`** (velocity calc only, position unaffected) → Not blocking; documented in the library, not modified.

## Migration Plan

1. Land code with defaults (old configs keep working: easing on by default at 180 deg/s, fade 250 ms, blink honoring existing `interval_on/off`).
2. Update both `data/hardware-*.json` files to declare the `animation` block explicitly.
3. Hot-reload (existing `reloadConfigs()`) picks up the new fields at runtime; no re-flash required for config-only changes.
4. Rollback: revert JSON and/or code; the previous commit restores instant behavior.

## Open Questions

- Is 1 Hz turn flashing acceptable, or should `interval_on/off` be tightened (e.g. 400/400) for a livelier signal? (Config-only change either way.)
- Should the tail light follow the head fade via read-back (D3) or fade independently on the same edge? Read-back chosen for v1; revisit if the tail visibly lags.
- Does any vehicle need `easing_speed_deg_s: 0` (instant aux servos)? Config knob exists; no shipped vehicle currently needs it.
