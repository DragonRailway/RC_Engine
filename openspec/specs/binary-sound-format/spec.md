# binary-sound-format Specification

## Purpose
TBD - created by archiving change binary-sound-pipeline. Update Purpose after archive.
## Requirements
### Requirement: 8-Byte PcmHeader container format
All sound asset files (`.pcm`) SHALL begin with an 8-byte packed header defined as:
- `magic`: 2 bytes containing `'R'`, `'P'` (`0x52`, `0x50`)
- `sampleRate`: 2 bytes (`uint16_t`, Little-Endian, e.g. 22050)
- `sampleCount`: 4 bytes (`uint32_t`, Little-Endian, number of audio samples)
Followed immediately by `sampleCount` contiguous bytes of signed 8-bit PCM audio samples (`int8_t`).

#### Scenario: Validating a .pcm header
- **WHEN** a `.pcm` sound file is inspected
- **THEN** the first 2 bytes are `"RP"`, the sampleRate is a valid rate (e.g. 22050), and the remaining file size exactly equals `sampleCount`

---

### Requirement: Zero-transient single-block sound loader
`ConfigParser::loadSoundSlot()` SHALL load `.pcm` sound files from LittleFS by reading the 8-byte header, allocating `ps_malloc(hdr.sampleCount)`, and reading the payload directly via a single `file.read()` call without allocating any intermediate JSON document ASTs in memory.

#### Scenario: Loading a sound slot from LittleFS
- **WHEN** `ConfigParser::loadSoundSlot("/sounds/vehicles/GP38_2/idle.pcm")` is called
- **THEN** the file is opened, header validated, buffer allocated in PSRAM, payload read in a single block read, and a populated `SoundSlot` returned with 0 bytes of temporary memory overhead

---

### Requirement: Asset build and validation tooling
`scripts/build_fs.py` SHALL stage `.pcm` sound files into the LittleFS filesystem image, and `scripts/validate_sounds.py` SHALL validate header integrity, byte lengths, non-silent audio, and clipping limits directly from `.pcm` files.

#### Scenario: Running validate_sounds.py on binary assets
- **WHEN** `python3 scripts/validate_sounds.py` is executed
- **THEN** all `.pcm` files across `configs/vehicle_configs/` are validated against the 8-byte `PcmHeader` specification

