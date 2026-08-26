# TrackLink V3 Audio Diagnostics Specification

## Requirement 1: Diagnostic Logging

**GIVEN** the firmware is built with `-DAUDIO_DEBUG`
**WHEN** the audio task runs
**THEN** it SHALL emit structured serial lines every 100 buffers with:
- Write count
- Bytes written
- Error code
- Task loop time (microseconds)

**AND** it SHALL emit `renderBlock()` maximum timing every time a new maximum is set.

## Requirement 2: Self-Test Mode

**GIVEN** the firmware is running
**WHEN** a serial command `AUDIO_SELFTEST sine|silence|impulse|sweep` is received
**THEN** the audio task SHALL switch to generating the specified test signal
**AND** it SHALL bypass the engine's `renderBlock()` entirely
**AND** it SHALL emit `[AUDIO_SELFTEST] mode=<mode>` on serial

## Requirement 3: Bounded DMA Timeout

**GIVEN** the audio task is writing to I2S
**WHEN** `i2s_channel_write()` is called
**THEN** it SHALL use `pdMS_TO_TICKS(50)` timeout instead of `portMAX_DELAY`
**AND** on timeout, the audio task SHALL log `[AUDIO_DIAG] DMA_TIMEOUT` and continue to the next iteration

## Requirement 4: FreeRTOS Task Monitoring

**GIVEN** the firmware is built with `-DAUDIO_DEBUG`
**WHEN** 5 seconds have elapsed since the last task check
**THEN** the audio task SHALL emit `[TASKS]` with `vTaskList()` output showing all tasks, their states, and stack usage

## Requirement 5: Production Safety

**GIVEN** the firmware is built without `-DAUDIO_DEBUG`
**WHEN** the audio task runs
**THEN** it SHALL have zero diagnostic overhead (no timing measurements, no serial output)
**AND** the bounded DMA timeout SHALL still apply (safety improvement)
