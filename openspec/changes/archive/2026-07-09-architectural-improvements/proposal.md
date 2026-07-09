# Proposal: Architectural Improvements

## What

Fix two high-priority bugs and refactor core data structures to make the SoundEngine more maintainable and correct.

## Why

The SoundEngine has grown to 24 voice slots with 60+ config fields. The current architecture has:

1. **A double DC offset bug** — `getNextSample()` adds +128 and `audioTask()` adds another 0-128 offset ramp. The ramp effect is only 0.4% of the signal, making pop prevention ineffective.

2. **ISR/main loop race condition** — `update()` writes voice state from the main loop while `getNextSample()` reads it from a timer ISR at 22,050 Hz. On ESP32-S3 dual-core, this causes torn reads on volume/active fields.

3. **SoundData struct with 48 fields** — Adding a new voice requires changes in 7 places across 4 files. This is error-prone and slow.

4. **Duplicated enum ordering** — `VoiceID` and `SoundIndex` must stay in sync manually. A mismatch causes sounds to map to wrong voices silently.

5. **Flat 60+ field Config struct** — All engine, sound, transmission, and feature config in one struct makes it hard to understand what belongs where.

## Scope

- Fix the DC offset bug in AudioOutput
- Fix ISR/main loop thread safety
- Refactor SoundData to use indexed arrays
- Unify VoiceID/SoundIndex enums
- Group Config into sub-structs
- Clean up minor issues (unused fields, hardcoded thresholds, signed/unsigned)

## Out of Scope

- Adding new voice slots or features
- Sample rate conversion
- Volume fade-in/fade-out (separate enhancement)
