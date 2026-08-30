## Why

Currently, sound samples are stored and parsed as ASCII JSON arrays (`[12, -4, 55, ...]`). Ingesting a single 22 KB sound sample via `ArduinoJson::deserializeJson` allocates a temporary ~350 KB `JsonDocument` AST in PSRAM, leading to 17x memory amplification, PSRAM fragmentation, and slow boot/reload times (~800ms per bundle). Furthermore, ASCII JSON bloats sound assets on disk (80 MB in repo, ~850 KB per vehicle bundle on LittleFS).

This change replaces JSON sound files with direct 8-byte header binary `.pcm` files, cutting sound load times by 20x, eliminating all transient PSRAM memory allocations (0 bytes temporary overhead), shrinking vehicle bundles on LittleFS by ~75% (to ~180 KB), and verifying the entire pipeline on `TRACKLINK_V3` hardware with a locomotive configuration.

## What Changes

- **8-Byte Binary Container Format (`.pcm`)**: Standardize all sound files as `PcmHeader` (magic `"RP"`, `uint16_t sampleRate`, `uint32_t sampleCount`) followed by raw `int8_t` PCM samples. **(BREAKING for sound files)**
- **Zero-Transient Ingest in `ConfigParser`**: `loadSoundSlot()` reads the 8-byte header, allocates `ps_malloc(sampleCount)`, and executes a single contiguous block read (`file.read()`) directly into the buffer.
- **Remove `SpiRamAllocator`**: Completely eliminate `SpiRamAllocator` and JSON parsing for audio files.
- **Asset Migration**: Convert all 1,371 sound assets in `configs/vehicle_configs/` to `.pcm` binaries via an automated conversion script, reducing repository sound assets from 80 MB to ~16 MB.
- **Tooling Updates**: Update `scripts/build_fs.py`, `scripts/validate_sounds.py`, and `scripts/host_dsp_test.py` to work natively with `.pcm` files.
- **Hardware Verification on `TRACKLINK_V3`**: Build and flash LittleFS on `TRACKLINK_V3` with a locomotive configuration, verifying boot telemetry, zero transient PSRAM usage, and glitch-free I2S audio playback.

## Capabilities

### New Capabilities
- `binary-sound-format`: 8-byte header binary `.pcm` format, direct block-read loader in `ConfigParser`, and asset build/validation tooling.
- `hardware-audio-verification`: On-device LittleFS deployment and I2S DAC playback validation on `TRACKLINK_V3` with a locomotive profile.

### Modified Capabilities
*(None. Vehicle physics, lighting, and high-level sound simulation APIs remain unchanged.)*

## Impact

- **`common/ConfigParser.h` / `ConfigParser.cpp`**: Simplified `loadSoundSlot()` with single block read; removed `SpiRamAllocator`.
- **`configs/vehicle_configs/`**: Sound files converted from `<slot>.json` to `<slot>.pcm`.
- **`scripts/build_fs.py`**: Stages `.pcm` files instead of `.json`.
- **`scripts/validate_sounds.py`**: Validates binary headers and signal statistics from `.pcm`.
- **`test/host_dsp/`**: Compiles native DSP test against `.pcm` files.
