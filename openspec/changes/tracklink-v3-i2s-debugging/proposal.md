# TrackLink V3 I2S Audio Debugging

## Problem Statement

The TrackLink V3 board produces **no audio output at all** — not even the horn when the engine is OFF. The engine state machine gets stuck in STARTING forever because the audio task deadlocks on `i2s_channel_write(..., portMAX_DELAY)`. The same firmware works correctly on MIKRO_V2.

## Current State

### What Works
- I2S peripheral initializes successfully (`i2s_new_channel`, `i2s_channel_init_std_mode`, `i2s_channel_enable` all return ESP_OK)
- Board boots, BLE advertising works, all peripherals functional
- Host VC test suite passes (25/25 tests)
- The I2S hardware path is functional when tested in isolation (see below)

### What Doesn't Work
- No sound output from the speaker in the main firmware
- Engine stuck in STARTING state forever
- `i2s_channel_write(..., portMAX_DELAY)` deadlocks on first write in the main firmware

### Key Discovery: I2S Hardware Works in Isolation

A minimal I2S sine wave test (`test_i2s_speaker/`) proves the hardware path is functional:

```
=== I2S Speaker Test (TRACKLINK_V3) ===
Pins: LRC=17, BCLK=18, DIN=21, SD=47
[1] I2S_SD (amp enable) set HIGH
[2] I2S channel allocated OK
[3] I2S standard mode initialized OK
[4] I2S channel enabled OK
--- All I2S setup succeeded. Playing 440 Hz sine wave... ---
[PLAYING] pos=22016,  write_us=5,    bytes=256, err=0  ✅
[PLAYING] pos=44032,  write_us=6,    bytes=256, err=0  ✅
[PLAYING] pos=66048,  write_us=7313, bytes=256, err=0  ✅ (DMA buffer full, waited ~7ms)
[PLAYING] pos=88064,  write_us=5,    bytes=256, err=0  ✅
[PLAYING] pos=110080, write_us=6,    bytes=256, err=0  ✅
[PLAYING] pos=132096, write_us=5,    bytes=256, err=0  ✅
[PLAYING] pos=154112, write_us=6966, bytes=256, err=0  ✅
```

All writes succeed (`err=0`, `bytes=256`). DMA drains normally. **This confirms the I2S peripheral, GPIO matrix routing, DAC amp enable, and DMA all work correctly on TrackLink V3.**

### What's Different

The minimal test runs as a single-threaded Arduino loop. The main firmware runs `audioTask` on Core 1 with FreeRTOS, BLE, RadioKit, and VehicleController all active.

## Root Cause Analysis

Since the I2S hardware works in isolation, the issue is in the main firmware's audio pipeline. Possible causes:

1. **Audio task never starts** — `AudioOutput::start()` might not be called, or the task is blocked on creation
2. **`renderBlock()` takes too long** — with 32 voices × 64 samples, something in the voice loop causes a stall, DMA buffer fills up and writes block
3. **Race condition** — engine state is corrupted by concurrent access between `startEngine()` (Core 0) and `renderBlock()` (Core 1)
4. **FreeRTOS task starvation** — BLE/RadioKit tasks on Core 1 starve the audio task of CPU time
5. **`portMAX_DELAY` deadlock** — even if the DMA drains slowly, `portMAX_DELAY` should eventually return; if it never does, something is fundamentally wrong with the I2S channel state

## Proposed Investigation

### Phase 1: Add Diagnostic Logging to Audio Task
- Add `AUDIO_DEBUG` build flag to the main firmware
- Log `i2s_channel_write` return value, bytes written, and timing
- Log `renderBlock` timing per call
- Log audio task state transitions

### Phase 2: Add Self-Test Serial Command
- Wire up `AudioOutput::setSelftestMode()` to a serial command
- Test: does the selftest sine play through the main firmware's audio task?
- If yes → issue is in `renderBlock()` / engine state
- If no → issue is in audio task startup or FreeRTOS scheduling

### Phase 3: Test with Bounded Timeout
- Replace `portMAX_DELAY` with `pdMS_TO_TICKS(50)` in `AudioOutput::audioTask`
- This prevents deadlock and lets the engine state machine advance even without audible output
- If the engine reaches RUNNING → the issue is purely I2S DMA related
- If the engine still stuck → something else is blocking the audio task

### Phase 4: Profile `renderBlock()` Timing
- Measure time for `renderBlock()` to complete with 32 active voices
- Check if it exceeds the 2.9ms real-time budget (256 bytes / 88200 bytes/sec)
- If too slow → optimize voice mixing or reduce active voice count

### Phase 5: Check FreeRTOS Task Scheduling
- Verify audio task priority (3) vs BLE/RadioKit tasks
- Check CPU core affinity (audio should be on Core 1)
- Monitor task switching with `vTaskList()`

## Success Criteria

- Audio plays through the speaker on TrackLink V3
- Engine state machine reaches RUNNING state
- No `portMAX_DELAY` deadlocks
- All existing tests still pass

## Files Affected

- `lib/SoundEngine/src/AudioOutput.h` — add diagnostic logging, bounded timeout
- `lib/SoundEngine/src/RcEngineSound.cpp` — add `renderBlock()` timing
- `src/main.cpp` — add serial command for self-test mode
- `platformio.ini` — add `AUDIO_DEBUG` build flag option

## Key Finding: Hardware Is NOT the Problem

The minimal I2S test (`test_i2s_speaker/`) conclusively proves the TrackLink V3 I2S hardware path works:

1. ✅ I2S peripheral initializes (channel, std mode, enable)
2. ✅ GPIO matrix routes signals correctly (pins 17/18/21/47)
3. ✅ DAC amp enable pin works (pin 47 HIGH)
4. ✅ DMA drains at expected rate (22050 Hz stereo 16-bit)
5. ✅ Speaker produces 440 Hz tone (user confirmed)

**This eliminates all hardware theories:** pin conflicts, GPIO matrix issues, clock source problems, DAC wiring faults, manufacturing defects.

**The issue is 100% in the main firmware's audio pipeline** — specifically in how `audioTask` interacts with FreeRTOS, BLE, RadioKit, and the engine state machine.

## Related

- Previous change: `locomotive-lights-and-reverser-dynamics` (completed)
- Audio debug tooling: `openspec/changes/archive/2026-08-14-audio-debug-tooling/` (completed)
- I2S test sketch: `test_i2s_speaker/` (standalone diagnostic — proves hardware works)
