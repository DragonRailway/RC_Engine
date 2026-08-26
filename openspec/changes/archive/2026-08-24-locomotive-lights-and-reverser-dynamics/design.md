## Context

The RadioKit firmware on TrackLink V3 controls model locomotives with physical LED outputs (`L1` to `L6`), motor drivers, and a DSP sound engine. While basic on/off lighting and direction switching exist, prototypical locomotive realism requires:
1. Direction-coupled lighting (headlights vs marker lights).
2. Soft incandescent lamp filament warm-up / cool-down transitions.
3. Smooth alternating cross-fade for ditch lights.
4. Momentum-safe reverser handling that prevents instant gear/motor destruction and models realistic train braking.

## Goals / Non-Goals

**Goals:**
- Implement directional handover: Reverser Forward (`1`) energizes forward Headlight (`L1`) and dims/extinguishes Rear Marker (`L2`); Reverser Reverse (`0`) reverses the roles.
- Implement software incandescent PWM ramping with asymmetric rising vs falling slopes (warm-up rate ~200ms, cool-down rate ~100ms).
- Implement sinusoidal/triangular soft cross-fade between left (`L4`) and right (`L5`) ditch lights.
- Implement reverser interlock: Zero throttle input demand and execute dynamic braking down to 0 km/h before applying torque in the opposing direction.

**Non-Goals:**
- Modifying road truck lighting or steering logic (this is strictly scoped to the `LOCOMOTIVE` vehicle profile).
- Changing wire protocol frame format (all controls use existing `loco_light`, `throttle_slider`, and `dir_switch` widgets).

## Decisions

### 1. Directional Lighting in `VehicleController`
- **Choice**: Evaluate `dir_switch` in the locomotive lighting mixer. When Bit 0 (Headlight) is set in `loco_light`:
  - If `dir_switch == 1` (Forward): drive `head_light` at configured max brightness, `tail_light` at 0% (or marker glow).
  - If `dir_switch == 0` (Reverse): drive `tail_light` at configured max brightness, `head_light` at marker glow / 0%.
- **Alternative considered**: Requiring the user to tap separate buttons for front and rear lights. Rejected because real locomotive reversers automatically orient directional lighting.

### 2. Incandescent PWM Ramping Engine
- **Choice**: Implement a non-blocking per-tick brightness slew rate limiter in `VehicleController.h` / `HardwareInit`.
  - Step size for fade-in: `maxDuty / (fade_in_ms / tick_interval_ms)`.
  - Step size for fade-out: `maxDuty / (fade_out_ms / tick_interval_ms)`.
- **Alternative considered**: Instant step change. Rejected because incandescent bulbs on model trains look much more realistic with filament warm-up.

### 3. Triangular Cross-Fade for Ditch Lights
- **Choice**: Oscillate a phase counter between `0` and `maxDuty`. `L4` output receives `phase`, `L5` output receives `maxDuty - phase`.
- **Alternative considered**: Hard square on/off flash. Rejected because soft oscillation mimics real incandescent sealed-beam cross-fading.

### 4. Reverser Flip & Dynamic Braking Interlock
- **Choice**: Track `s_activeDirection`. When `dir_switch` state changes while current `speed > 0`:
  1. Immediately clamp `throttle_slider` internal target to `0`.
  2. Maintain `s_activeDirection` until vehicle comes to a full stop (`speed == 0`).
  3. Play dynamic braking / air brake audio until stopped.
  4. Once speed reaches 0, update `s_activeDirection = dir_switch` and allow new throttle commands to apply.

## Risks / Trade-offs

- [Latency on light toggles due to fading] → Fade duration is capped at ~150-250ms, preserving fast responsive feel while providing filament realism.
- [App slider position vs firmware throttle target] → When reverser flips, firmware clamps target throttle to 0. Firmware telemetry updates the status display so the user observes braking and standstill before reapplying throttle.
