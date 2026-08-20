# Design: Fix Sound Engine Simulation

## Overview
This design resolves three DSP and FreeRTOS synchronization issues in `RcEngineSound.cpp`:
1. **Knock trigger pointer circular wrap-around** in `RcEngineSound::update()`
2. **Idle/Rev cross-fade proportion mapping** in `RcEngineSound::update()`
3. **Per-voice critical section spinlock overhead** in `RcEngineSound::getNextSample()`

---

## Detailed Design

### 1. Circular Wraparound for Knock Trigger
In `RcEngineSound::update()`:
```cpp
if (state == RUNNING && !engineMuted && sounds.slots[KNOCK].samples && cfg.sound.knock > 0 && cfg.engine.knockInterval > 0) {
    uint32_t totalSamples = sounds.slots[IDLE].sampleCount;
    uint32_t knockIntervalSamples = totalSamples / cfg.engine.knockInterval;
    if (knockIntervalSamples > 0) {
        uint32_t idlePos = (uint32_t)voices[IDLE].position;
        uint32_t elapsed = (idlePos >= lastKnockTriggerSample) 
            ? (idlePos - lastKnockTriggerSample) 
            : (totalSamples - lastKnockTriggerSample + idlePos);

        if (elapsed >= knockIntervalSamples) {
            lastKnockTriggerSample = idlePos;
            curKnockCylinder++;
            if (curKnockCylinder > cfg.engine.knockInterval) curKnockCylinder = 1;
            voices[KNOCK].active = true;
            voices[KNOCK].position = 0;
        }
    }
}
```

### 2. Monotonic Idle/Rev Cross-fade Proportion
In `RcEngineSound::update()`:
```cpp
int16_t idleProportion = 100;
if (state == RUNNING && !engineMuted) {
    voices[IDLE].active = (sounds.slots[IDLE].samples && sounds.slots[IDLE].sampleCount > 0);
    voices[REV].active = (sounds.slots[REV].samples && sounds.slots[REV].sampleCount > 0);
    if (currentRpmFixed <= cfg.engine.idleEndPoint) {
        idleProportion = 100;
    } else if (currentRpmFixed >= cfg.engine.revSwitchPoint) {
        idleProportion = 0;
    } else {
        idleProportion = map(currentRpmFixed, cfg.engine.idleEndPoint, cfg.engine.revSwitchPoint, 100, 0);
        idleProportion = constrain(idleProportion, 0, 100);
    }
} else {
    voices[IDLE].active = false;
    voices[REV].active = false;
}
```

### 3. Batched Spinlock Synchronization in `getNextSample()`
Currently, `getNextSample()` calls `portENTER_CRITICAL` and `portEXIT_CRITICAL` for every active voice individually.
Instead:
- Take a single critical section read snapshot at the top of `getNextSample()`.
- Accumulate the mix and advance voice positions into a local array.
- Write back all changed positions/active flags under a single `portENTER_CRITICAL` / `portEXIT_CRITICAL` block at the end of the voice loop in `getNextSample()`.
- Reduces spinlock acquisitions from up to 33 per sample down to at most 2 per sample (or ~94% reduction in critical section overhead).
