## Context

In `RC_Engine`, `HardwareConfig` defines board peripherals. Unlike other peripherals (lights, steering servos, drive motors) which are only instantiated when declared in `hardware-<BOARD>.json`, audio output hardware (`AudioOutput`) was started unconditionally at boot.

Additionally, sound asset loading consumed memory and file I/O even if a board was intentionally silent or sound files were absent.

## Goals / Non-Goals

**Goals:**
- Gate `AudioOutput::begin()` behind both `hwConfig.sound.configured == true` and `loadedSoundCount > 0`.
- Bypass sound loading in `ConfigParser::loadSounds()` if `hwConfig.sound.configured == false`.
- Keep `RcEngineSound` vehicle physics simulation (RPM, flywheel, torque converter, shifting) running continuously regardless of audio output state.
- Update documentation and schemas to reflect the opt-in sound model.

**Non-Goals:**
- Changing PCM audio sample rate (remains 22,050 Hz).
- Changing the sound JSON asset file format or storage schema.
- Providing backward-compatibility fallbacks for unconfigured sound.

## Decisions

1. **Explicit Hardware Opt-in (`sound.configured`)**:
   - `HardwareConfig::Sound::configured` defaults to `false`.
   - Only parsed if `"sound"` or `"SOUND"` is an explicit JSON object in `hardware-<BOARD>.json`.
   - If not configured, `AudioOutput` is never initialized.

2. **Asset Presence Check (`loadedSoundCount > 0`)**:
   - `ConfigParser::loadSounds()` returns the count of loaded sound slots.
   - If 0 sounds are resolved (missing or empty asset files), `AudioOutput::begin()` is skipped.

3. **Physics Simulator Isolation**:
   - `engine.begin(profile.sounds, profile.config)` is always called so RPM curves, shift timers, and start/stop state machines function for `VehicleController`.

## Risks / Trade-offs

- [Silent vehicle on misconfigured JSON] → If a user forgets to add `"sound"` to a custom hardware JSON, sound will not play. Mitigation: Log explicit `[HardwareInit] Audio hardware: DISABLED (omitted from hardware config)` in serial output.
- [Missing sound assets] → If sound files are missing from flash, motor control continues normally without I2S errors.
