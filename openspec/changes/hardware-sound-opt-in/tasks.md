## 1. Config & Parser Updates

- [x] 1.1 Update `common/Config.h` to ensure `HardwareConfig::Sound::configured` defaults to `false`.
- [x] 1.2 Update `common/ConfigParser.h` so `ConfigParser::loadSounds()` returns the loaded sound count and bypasses loading when sound is not configured in hardware.

## 2. Firmware Integration & Initialization

- [x] 2.1 Update `src/main.cpp` boot sequence and hot-reload handlers to gate `AudioOutput::begin(&engine)` on `hwConfig.sound.configured && loadedSounds > 0`.
- [x] 2.2 Add clear serial status logging for audio hardware initialization (ENABLED vs DISABLED with reason).

## 3. Schema & Documentation

- [x] 3.1 Update `configs/schemas/hardware_config.schema.json` to reflect `"sound"` as an optional opt-in hardware block.
- [x] 3.2 Update `GUIDE/HARDWARE_CONFIG.md` to document the opt-in sound model and asset requirement.

## 4. Verification & Testing

- [x] 4.1 Run host vehicle controller tests (`python3 scripts/host_vc_test.py`).
- [x] 4.2 Validate all hardware configs against updated schema (`python3 scripts/validate_configs.py`).
- [x] 4.3 Build firmware for `TRACKLINK_V3` (`pio run -e TRACKLINK_V3`).
- [x] 4.4 Flash firmware to the connected TrackLink V3 board and verify over Remote API.
