# Tasks: TrackLink V3 I2S Audio Debugging

## Phase 1: Diagnostic Infrastructure

- [ ] 1.1 Add `AUDIO_DEBUG` build flag to `platformio.ini` for TRACKLINK_V3 environment
- [ ] 1.2 Add diagnostic logging to `AudioOutput::audioTask` (write count, bytes, error, timing)
- [ ] 1.3 Add `renderBlock()` timing instrumentation to `RcEngineSound.cpp`
- [ ] 1.4 Wire up `AUDIO_SELFTEST` serial command in `main.cpp`
- [ ] 1.5 Add FreeRTOS task monitoring with `vTaskList()` output

## Phase 2: Safety Fix

- [ ] 2.1 Replace `portMAX_DELAY` with `pdMS_TO_TICKS(50)` in `AudioOutput::audioTask`
- [ ] 2.2 Add timeout logging (`[AUDIO_DIAG] DMA_TIMEOUT`)
- [ ] 2.3 Verify engine state machine advances even when DMA times out

## Phase 3: Diagnostics Collection

- [ ] 3.1 Build firmware with `-DAUDIO_DEBUG`
- [ ] 3.2 Flash to TrackLink V3 and capture serial output
- [ ] 3.3 Start engine and observe diagnostic lines
- [ ] 3.4 Test self-test mode (`AUDIO_SELFTEST sine`)
- [ ] 3.5 Analyze `renderBlock()` timing vs 2.9ms real-time budget

## Phase 4: Root Cause Identification

- [ ] 4.1 Determine if audio task is actually running (check task list)
- [ ] 4.2 Determine if `renderBlock()` exceeds real-time budget
- [ ] 4.3 Determine if BLE/RadioKit tasks starve audio task
- [ ] 4.4 Determine if engine state is corrupted by race condition
- [ ] 4.5 Document findings and root cause

## Phase 5: Fix Implementation

- [ ] 5.1 Implement fix based on root cause findings
- [ ] 5.2 Verify fix with diagnostic logging
- [ ] 5.3 Remove `AUDIO_DEBUG` flag and verify production build works
- [ ] 5.4 Run full test suite to ensure no regressions
- [ ] 5.5 Archive this change and sync specs
