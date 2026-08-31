## 1. Throttle Volume Slew-Rate Inertia

- [x] 1.1 Add `currentThrottleFaded` state variable in `RcEngineSound.h` and initialize to 0.
- [x] 1.2 In `RcEngineSound::update()`, implement asymmetrical slew-rate filter for throttle volume (instant attack on rise, smooth exponential decay on release).
- [x] 1.3 Update IDLE and REV dynamic volume calculations to use `currentThrottleFaded` instead of instantaneous slider `throttlePercent`.

## 2. Phase-Locked Dual-Voice Engine Synchronization

- [x] 2.1 In `RcEngineSound::renderBlock()`, phase-lock `voices[REV]` playhead position to `voices[IDLE]` normalized cycle phase when both voices are active.
- [x] 2.2 Synchronously wrap both IDLE and REV sample pointers at cylinder cycle loop boundaries in `advanceVoice()`.

## 3. Cycle-Quantized Jake Brake & Smooth Ducking

- [x] 3.1 Refactor jake brake state machine in `RcEngineSound` with `jakeBrakeRequest` and cycle-completion deactivation in `renderBlock()`.
- [x] 3.2 Replace `engineMuted` hard zero clamp with smooth 20% ducking of primary engine rev voice during jake braking.

## 4. Verification & Testing

- [x] 4.1 Update `host_dsp_harness` with phase-locking and throttle decay test assertions.
- [x] 4.2 Run host DSP and host vehicle controller test suites.
- [x] 4.3 Build and flash `MIKRO_V2` board, and verify seamless throttle release transitions over serial and audio.
