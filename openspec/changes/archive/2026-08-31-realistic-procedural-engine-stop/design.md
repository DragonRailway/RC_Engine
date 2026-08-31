## Context

When an engine stop is requested (`stopEngine()`), `RcEngineSound` currently freezes `stopPitchFactor` at `1.0f` due to invalid clamping, attenuates amplitude using steep reciprocal integer division (`scaled / attenuator`), cuts off diesel knock abruptly, and waits 3.2 seconds before triggering the parking brake air release sound.

## Goals / Non-Goals

**Goals:**
- Implement continuous inertial flywheel pitch coast-down from 1.0 down to `0.18f`.
- Implement a smooth, linear/cosine volume envelope avoiding the 1/x reciprocal cliff.
- Maintain cylinder knock firing during `STOPPING`, allowing firing cadence to naturally slow down and volume to fade with the engine.
- Rapidly fade turbo whistle over the first 50% of the stop duration.
- Provide type-driven default durations (1.4s for Trucks, 2.8s for Locomotives, 1.8s for Excavators) with optional `"stop_duration"` in `vehicle.json`.
- Hand off immediately to `PARKING_BRAKE` when the spin-down completes with zero dead silence.

**Non-Goals:**
- Requiring extra recorded shutdown sample files for existing vehicle profiles.

## Decisions

### 1. Elapsed Time Tracking & Inertial Pitch Curve
In `RcEngineSound.h` / `RcEngineSound.cpp`:
- Add `uint32_t stopStartMillis = 0;` and `uint16_t stopDurationMs = 1400;` to `RcEngineSound`.
- When transitioning from `RUNNING` to `STOPPING`:
  `stopStartMillis = now;`
  `stopDurationMs = cfg.engine.stopDuration > 0 ? cfg.engine.stopDuration : 1400;`
- In `update()`:
  ```cpp
  float progress = (float)(now - stopStartMillis) / (float)stopDurationMs;
  if (progress > 1.0f) progress = 1.0f;
  stopPitchFactor = 0.18f + (1.0f - 0.18f) * powf(1.0f - progress, 1.2f);
  pitchFactor = stopPitchFactor;
  stopVolume = (uint8_t)((1.0f - progress) * 100.0f);
  ```
- When `progress >= 1.0f`:
  Transition immediately to `PARKING_BRAKE` (if sample exists) or `OFF`.

### 2. Mixer Amplitude Envelope in `renderBlock()`
In `renderBlock()`:
- Snapshot `currentStopVolume` under the voice mutex.
- For pitch-shifted voices in `STOPPING`:
  `scaled = (scaled * currentStopVolume) / 100;`
- For `TURBO`:
  `float turboProgress = min(1.0f, progress * 2.0f);`
  `voices[TURBO].volume = (uint8_t)(cfg.sound.turbo * (1.0f - turboProgress));`

### 3. Knock Cadence Continuity
In `RcEngineSound.cpp`:
- Enable knock calculation when `(state == RUNNING || state == STOPPING) && !engineMuted`:
  Because `voices[IDLE].position` advances with `step = engineStep` (which is `stopPitchFactor`), the loop position advances progressively slower in real time.
  The knock interval condition `elapsed >= knockIntervalSamples` naturally takes longer between firings, producing authentic deceleration chugs.
- Scale knock volume with `stopVolume`.

### 4. Configuration Support
In `common/Config.h`:
- Add `uint16_t stopDuration = 1400;` to `EngineConfig`.
In `common/ConfigParser.cpp`:
- In `parseEngine()`:
  ```cpp
  uint16_t defaultStop = (cfg.type == RcEngineSound::VEHICLE_LOCOMOTIVE) ? 2800 :
                         (cfg.type == RcEngineSound::VEHICLE_EXCAVATOR) ? 1800 : 1400;
  cfg.engine.stopDuration = eng["stop_duration"] | eng["STOP_DURATION"] | defaultStop;
  ```

## Risks / Trade-offs

- **[Risk]** Millis overflow or abrupt frame updates:
  - **Mitigation:** Unsigned subtraction `now - stopStartMillis` is immune to rollover. `progress` is strictly clamped to `[0.0, 1.0]`.
- **[Risk]** Host test harness assertions for STOPPING:
  - **Mitigation:** Update host tests (`host_dsp_driver.cpp`, `host_vc_driver.cpp`) to verify continuous pitch deceleration down to 0.18f.
