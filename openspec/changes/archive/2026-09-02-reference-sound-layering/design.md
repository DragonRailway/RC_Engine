## Context

The upstream reference sound engine (`Rc_Engine_Sound_ESP32`) mixes `IDLE` and `REV` sound samples concurrently rather than muting or crossfading `REV` to zero across engine RPM breakpoints. In our earlier implementation, an artificial `idleProportion` crossfade formula forced the `REV` voice volume to zero below `idleEndPoint` (300 RPM $\to$ 50 RPM), creating an abrupt volume drop whenever the vehicle decelerated or coasted.

## Goals / Non-Goals

**Goals:**
- Eliminate abrupt REV audio cutoffs during engine deceleration.
- Implement reference-parity concurrent layering of IDLE and REV voices.
- Use cycle-quantized Jake brake muting and un-muting on sample loop boundary.

**Non-Goals:**
- Modifying raw PCM sound assets.
- Changing steering, lighting, or transmission shifting logic.

## Decisions

### Decision: Pure Reference Layered Volume Scaling
- **Choice**: Scale IDLE and REV volume directly with dynamic throttle load:
  ```cpp
  int32_t idleVol = map(throttlePct, 0, 100, cfg.sound.engineIdle, cfg.sound.fullThrottle);
  int32_t revVol  = map(throttlePct, 0, 100, cfg.sound.engineRev,  cfg.sound.fullThrottle);
  voices[IDLE].volume = (uint8_t)constrain((cfg.sound.idle * idleVol / 100), 0, 100);
  voices[REV].volume  = (uint8_t)constrain((cfg.sound.rev  * revVol  / 100), 0, 100);
  ```
- **Rationale**: IDLE and REV sound loops represent the same number of ignition cycles and play concurrently in the reference design. When coasting down, both voices naturally sustain their pitch tracking with the flywheel without artificial volume cliffs.

### Decision: Cycle-Quantized Jake Brake Disengagement
- **Choice**: Keep base engine voices muted while Jake braking is active. When `jakeBrakeRequest` drops to false, disengage Jake brake and un-mute engine voices only upon completing the current loop cycle (`wrapped == true`).
- **Rationale**: Prevents clicking and audio pops from interrupting the Jake brake waveform mid-sample.

## Risks / Trade-offs

- **[Mix Dynamic Range]** Playing IDLE and REV concurrently adds signal energy → *Mitigation*: Existing group multipliers (`0.8f`) and rational soft-knee analog saturation limiter prevent dynamic range overflow and clipping.
