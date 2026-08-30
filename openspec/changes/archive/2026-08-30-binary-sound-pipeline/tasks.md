## 1. Asset Migration & Tools

- [x] 1.1 Create `scripts/convert_sounds_to_pcm.py` and convert all 1,371 sound files in `configs/vehicle_configs/` to `.pcm` binaries
- [x] 1.2 Update `scripts/validate_sounds.py` to validate `.pcm` headers, sample rates, and signal stats
- [x] 1.3 Update `scripts/build_fs.py` to stage `.pcm` sound assets into LittleFS images

## 2. Firmware Ingestion (`ConfigParser`)

- [x] 2.1 Define `struct PcmHeader` and update `ConfigParser::loadSoundSlot()` in `common/ConfigParser.cpp` and `common/ConfigParser.h` to use direct single-read binary loader
- [x] 2.2 Update `ConfigParser::loadSounds()` path resolution from `.json` to `.pcm`
- [x] 2.3 Remove `SpiRamAllocator` from `common/ConfigParser.h` and `common/ConfigParser.cpp`

## 3. Host DSP & Regression Harness

- [x] 3.1 Update `test/host_dsp/host_dsp_driver.cpp` and `scripts/host_dsp_test.py` to load `.pcm` files
- [x] 3.2 Run `host_dsp_test.py`, `host_vc_test.py`, and `validate_configs.py` on x86

## 4. Hardware Verification (TRACKLINK_V3 Locomotive)

- [x] 4.1 Compile firmware across board environments (`pio run -e TRACKLINK_V3 -e MIKRO_V2 -e GTRACK`)
- [x] 4.2 Build and verify locomotive LittleFS bundle deployment (`python3 scripts/build_fs.py --board TRACKLINK_V3 --vehicle GP38_2`)
- [x] 4.3 Verify on-device boot telemetry, zero transient PSRAM usage, and I2S DAC audio playback
