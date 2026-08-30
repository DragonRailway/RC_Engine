## Context

Sound files currently exist as ASCII JSON arrays `[12, -4, 55, ...]`. Loading a single sound file via `ArduinoJson::deserializeJson()` creates an in-memory AST variant tree in PSRAM requiring 16 bytes of metadata per sample (~350 KB temporary allocation for a 22 KB sound). This causes PSRAM heap fragmentation, slow boot/reload times (~800ms per vehicle), and inflates filesystem images (~850 KB on LittleFS).

## Goals / Non-Goals

**Goals:**
- Define a packed 8-byte binary container header (`PcmHeader`) for `.pcm` sound assets.
- Convert all 1,371 sound files in `configs/vehicle_configs/` to binary `.pcm` files, shrinking repository assets from 80 MB to ~16 MB.
- Replace `ConfigParser::loadSoundSlot()` with a direct single-read binary loader using `ps_malloc()`, achieving 0 bytes of temporary memory allocation.
- Remove `SpiRamAllocator` and all JSON parsing logic for sound files.
- Update `scripts/build_fs.py`, `scripts/validate_sounds.py`, and `test/host_dsp/` to work natively with `.pcm` files.
- Perform hardware verification on `TRACKLINK_V3` board with a locomotive bundle.

**Non-Goals:**
- Audio compression (ADPCM, MP3, Opus): 8-bit PCM @ 22,050 Hz is already ideal for low-latency zero-decoding playback on ESP32-S3.

## Decisions

### 1. 8-Byte Packed PcmHeader
```cpp
#pragma pack(push, 1)
struct PcmHeader {
    char     magic[2];    // "RP" (0x52, 0x50)
    uint16_t sampleRate;  // Little-Endian (default: 22050)
    uint32_t sampleCount; // Little-Endian byte count of PCM data
};
#pragma pack(pop)
```
- Total header size: Exactly 8 bytes (aligned).
- Payload: `sampleCount` contiguous bytes of signed 8-bit PCM (`int8_t`).

### 2. Streamlined Direct-Read Loader (`ConfigParser.cpp`)
```cpp
SoundSlot ConfigParser::loadSoundSlot(const char* path) {
    File file = LittleFS.open(path, "r");
    if (!file) return SoundSlot();

    PcmHeader hdr;
    if (file.read((uint8_t*)&hdr, sizeof(hdr)) != sizeof(hdr) ||
        hdr.magic[0] != 'R' || hdr.magic[1] != 'P') {
        file.close();
        return SoundSlot();
    }

    int8_t* buffer = (int8_t*)ps_malloc(hdr.sampleCount);
    if (!buffer) {
        file.close();
        return SoundSlot();
    }

    file.read((uint8_t*)buffer, hdr.sampleCount);
    file.close();

    SoundSlot slot;
    slot.samples = buffer;
    slot.sampleCount = hdr.sampleCount;
    slot.sampleRate = hdr.sampleRate;
    return slot;
}
```

### 3. Removal of `SpiRamAllocator`
`SpiRamAllocator` was only needed to route large `JsonDocument` AST allocations to PSRAM during sound parsing. With sound files loaded directly into `ps_malloc()`, `SpiRamAllocator` is removed.

### 4. Automated Migration Script
`scripts/convert_sounds_to_pcm.py`:
- Parses each `.json` sound file.
- Writes `<slot>.pcm` using `struct.pack('<2sHI', b'RP', sampleRate, sampleCount) + bytes(samples)`.
- Deletes the original `.json` sound files.

## Hardware Verification Plan (TRACKLINK_V3 + Locomotive)

1. **Host Verification**:
   - `validate_sounds.py` checks all converted `.pcm` files.
   - `host_dsp_test.py` validates DSP synthesis against `.pcm` files with 0 glitches.
2. **Build & Flash**:
   - Deploy locomotive LittleFS image: `python3 scripts/build_fs.py --board TRACKLINK_V3 --vehicle GP38_2`.
   - Compile firmware: `pio run -e TRACKLINK_V3`.
3. **On-Device Validation**:
   - Monitor boot logs: verify instantaneous loading (~20–40ms) and clean Free PSRAM report.
   - Test throttle notches, dynamic horn, and bell playback through MAX98357A I2S DAC.
