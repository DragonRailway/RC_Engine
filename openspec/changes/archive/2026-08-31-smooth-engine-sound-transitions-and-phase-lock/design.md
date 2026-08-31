## Context

In `RcEngineSound`, the rev voice stops abruptly upon throttle release due to:
1. Hard-clamping `voices[REV].volume = 0` immediately when `engineMuted` activates on jake brake condition.
2. Sudden volume drop from `throttleVol` tracking raw touchscreen slider input rather than flywheel inertia.
3. Asynchronous free-running playheads between `IDLE` and `REV` voices causing phase drifting and cancellation.

This design adapts the proven reference principles from TheDIYGuy99`s `Rc_Engine_Sound_ESP32` to the modern 16-bit block-based DSP pipeline.

## Goals / Non-Goals

**Goals:**
- **Phase-Locked Ignition Sync**: Keep IDLE and REV playhead positions aligned to the same normalized cylinder phase [0.0, 1.0), wrapping at ignition cycle boundaries.
- **Slew-Rate Throttle Volume Inertia**: Smoothly ramp engine volume up on throttle and decay gradually down on release (`currentThrottleFaded`).
- **Cycle-Quantized Jake Brake**: Complete the full jake brake sample cycle before transitioning back to the un-muted engine rev voice, and duck rather than zero the background engine body.

**Non-Goals:**
- Modifying underlying PCM sound sample files or littlefs formats.
- Changing vehicle transmission gear ratios or physical motor control loops.

## Decisions

### 1. Phase-Locked Dual-Voice Synchronization
- **Decision**: In `renderBlock()`, when both IDLE and REV voices are active, derive REV`s normalized phase directly from IDLE`s playhead position (`revPos = idlePos * ((float)revCount / (float)idleCount)`) or advance them together using a common cycle phase.
- **Rationale**: Both samples represent identical cylinder stroke counts recorded under different loads. Locking phase eliminates phasing artifacts and ensures seamless cross-fading.

### 2. Slew-Rate Throttle Volume Tracking (`currentThrottleFaded`)
- **Decision**: Introduce a floating-point `throttleVolumeFaded` updated every 20ms tick in `RcEngineSound::update()`. Instant upward tracking for crisp throttle punch, smooth exponential decay downward matching engine deceleration rate.
- **Rationale**: Prevents sudden 3x volume drops when the driver releases the throttle while engine RPM is still screaming high.

### 3. Jake Brake Loop Quantization & Engine Ducking
- **Decision**: Track `jakeBrakeRequest` vs `jakeBrakeActive`. When `jakeBrakeRequest` ends, keep `jakeBrakeActive` engaged until the jake brake sample finishes its loop cycle. Duck primary engine volume to ~20% during jake braking rather than hard muting to 0%.
- **Rationale**: Completely eliminates jarring audio clicks and sudden pop-ins during compression braking.

## Risks / Trade-offs

- **[Risk]** Differing sample rates between idle.pcm and rev.pcm in third-party sound sets.
  → **Mitigation**: Calculate phase scaling factor `((float)revCount / (float)idleCount)` so phase remains locked even if sample lengths differ slightly.
- **[Risk]** Jake brake hanging if sample count is zero or corrupted.
  → **Mitigation**: Verify `v.count > 0` before quantizing loop exit; fallback to immediate exit on invalid buffers.
