# Design: TrackLink V3 I2S Audio Debugging

## Context

The TrackLink V3 board produces no audio output despite identical firmware working on MIKRO_V2. A minimal I2S sine wave test proves the hardware path is functional, so the issue lies in the main firmware's audio pipeline.

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    MAIN FIRMWARE AUDIO PIPELINE             │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  Core 0 (Main Loop)                Core 1 (audioTask)       │
│  ─────────────────                 ──────────────────        │
│  VehicleController::update()       while(true) {             │
│    eState = getState() ──reads──►    currentState = state;   │
│    if (OFF) startEngine() ─wr►       │                      │
│      state = STARTING                │  renderBlock(buf, 64) │
│      startPos = 0  ← NO MUTEX!      │   ├─ voice mixing     │
│                                      │   └─ state writeback  │
│                                      │  i2s_channel_write(    │
│                                      │    portMAX_DELAY)     │
│  }                                   }                       │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                    MINIMAL I2S TEST                         │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  Core 0 (Main Loop only)                                   │
│  ─────────────────────────                                  │
│  loop() {                                                   │
│    generate sine buffer                                     │
│    i2s_channel_write(pdMS_TO_TICKS(100))                    │
│  }                                                          │
│                                                             │
│  ✅ Works: DMA drains, speaker produces 440 Hz tone        │
└─────────────────────────────────────────────────────────────┘
```

## Key Differences Between Working Test and Broken Firmware

| Aspect | Minimal Test | Main Firmware |
|--------|-------------|---------------|
| Threading | Single-core loop | FreeRTOS task on Core 1 |
| DMA timeout | `pdMS_TO_TICKS(100)` | `portMAX_DELAY` |
| Voice mixing | None (pure sine) | 32 voices × 64 samples |
| Concurrent tasks | None | BLE, RadioKit, VehicleController |
| Engine state | N/A | OFF → STARTING → RUNNING |

## Diagnostic Strategy

### Step 1: Add Diagnostic Logging

Add `AUDIO_DEBUG` build flag to emit structured serial lines from the audio task:

```cpp
// In AudioOutput::audioTask, after i2s_channel_write:
#ifdef AUDIO_DEBUG
static uint32_t writeCount = 0;
writeCount++;
if (writeCount % 100 == 0) {
    uint32_t elapsed_us = (uint32_t)(t_i2s - t_start);
    Serial.printf("[AUDIO_DIAG] write=%lu, bytes=%d, err=%d, task_us=%lu\n",
                  writeCount, (int)bytes_written, err, elapsed_us);
}
#endif
```

### Step 2: Wire Up Self-Test Mode

Add serial command handler in `main.cpp`:

```cpp
// In serial command processing:
if (cmd.startsWith("AUDIO_SELFTEST")) {
    if (cmd.endsWith("sine")) AudioOutput::setSelftestMode(AudioOutput::SELFTEST_SINE);
    else if (cmd.endsWith("silence")) AudioOutput::setSelftestMode(AudioOutput::SELFTEST_SILENCE);
    // etc.
}
```

### Step 3: Test with Bounded Timeout

Replace `portMAX_DELAY` with `pdMS_TO_TICKS(50)`:

```cpp
// BEFORE:
i2s_channel_write(tx_handle, buffer, sizeof(buffer), &bytes_written, portMAX_DELAY);

// AFTER:
i2s_channel_write(tx_handle, buffer, sizeof(buffer), &bytes_written, pdMS_TO_TICKS(50));
```

This prevents deadlock and lets us observe:
- If timeout fires → DMA is stuck (but task stays alive)
- If writes succeed → DMA is working, issue is elsewhere

### Step 4: Profile renderBlock()

Add timing instrumentation:

```cpp
void RcEngineSound::renderBlock(int16_t* buf, size_t frames) {
    int64_t t0 = esp_timer_get_time();
    // ... existing code ...
    int64_t t1 = esp_timer_get_time();
#ifdef AUDIO_DEBUG
    static uint32_t maxRenderUs = 0;
    uint32_t renderUs = (uint32_t)(t1 - t0);
    if (renderUs > maxRenderUs) {
        maxRenderUs = renderUs;
        Serial.printf("[RENDER] new max: %lu us\n", renderUs);
    }
#endif
}
```

### Step 5: Check FreeRTOS Scheduling

Add task diagnostics:

```cpp
// In audioTask, periodically:
#ifdef AUDIO_DEBUG
static uint32_t lastTaskCheck = 0;
uint32_t now = millis();
if (now - lastTaskCheck > 5000) {
    lastTaskCheck = now;
    char buf[256];
    vTaskList(buf);
    Serial.printf("[TASKS]\n%s\n", buf);
}
#endif
```

## Risk Mitigation

- All diagnostic code gated behind `AUDIO_DEBUG` flag — zero impact on production builds
- Bounded timeout prevents deadlock but may cause audio stuttering if DMA is slow
- Self-test mode bypasses engine entirely — safe for hardware verification
- Profiling uses integer math only — minimal overhead

## Open Questions

1. Is the audio task actually running? (Need to verify task creation succeeds)
2. Does `renderBlock()` complete within the 2.9ms real-time budget?
3. Is the BLE/RadioKit stack starving the audio task of CPU time?
4. Does the self-test sine play through the main firmware's audio task?

## Next Steps

After diagnostic logging is in place:
1. Build with `-DAUDIO_DEBUG`
2. Flash to TrackLink V3
3. Start engine and observe serial output
4. Analyze timing data to identify bottleneck
5. Fix root cause based on findings
