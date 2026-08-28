## Why

Currently, audio output hardware (I2S DMA and audio processing tasks) is initialized unconditionally at boot even if `"sound"` is omitted from the hardware JSON config or if no sound sample files exist. Like all other hardware peripherals (lights, servos, drive motors, aux), sound output hardware must follow a strict opt-in model: if `"sound"` is not defined in `hardware-<BOARD>.json` or if no sound sample files are available, audio hardware is completely disabled.

Crucially, the vehicle engine physics simulation (RPM, flywheel inertia, clutch engagement, torque converter, and start/stop states) depends on `vehicle.json` and runs continuously to drive motor control regardless of whether audio output is enabled.

## What Changes

- **Hardware Sound Opt-in**: The audio subsystem (`AudioOutput::begin()`) is only initialized if `hwConfig.sound.configured == true` AND at least one sound sample is successfully loaded (`loadedSoundCount > 0`).
- **Bypass Sound Loading on Unconfigured Hardware**: If `hwConfig.sound.configured == false`, sound asset loading (`ConfigParser::loadSounds`) is bypassed, saving ~100–500 KB RAM and flash I/O time on silent boards.
- **Physics Independence**: Engine simulation in `RcEngineSound` and `VehicleController` operates continuously regardless of hardware sound presence.
- **Schema & Documentation**: Updated `hardware_config.schema.json` and documentation to establish `"sound"` as an explicit opt-in hardware block.

## Capabilities

### Modified Capabilities
- `sound-engine-config`: Audio hardware initialization requires explicit hardware declaration (`sound.configured`) and valid sound assets, while vehicle simulation remains independent.

## Impact

- `common/Config.h`: `HardwareConfig::Sound::configured` is strictly `false` by default unless parsed from JSON.
- `common/ConfigParser.h`: `ConfigParser::loadSounds()` only runs if sound is configured, and reports loaded sample count.
- `src/main.cpp`: Gated `AudioOutput::begin(&engine)` on `hwConfig.sound.configured && loadedSounds > 0`.
- `configs/schemas/hardware_config.schema.json`: Document `"sound"` as optional opt-in block.
- `GUIDE/HARDWARE_CONFIG.md`: Document sound hardware opt-in behavior.
