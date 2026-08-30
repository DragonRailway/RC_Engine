## ADDED Requirements

### Requirement: TRACKLINK_V3 Locomotive LittleFS image deployment
`scripts/build_fs.py` SHALL build and flash a LittleFS image targeting `--board TRACKLINK_V3` with a locomotive vehicle bundle containing `.pcm` sound assets.

#### Scenario: Building and deploying locomotive LittleFS bundle
- **WHEN** `python3 scripts/build_fs.py --board TRACKLINK_V3 --vehicle GP38_2` is executed
- **THEN** a LittleFS binary image containing `hardware-TRACKLINK_V3.json`, `vehicle-config.json`, and all locomotive `.pcm` sound assets is assembled and flashed to the board

---

### Requirement: On-device sound loading and audio playback
When booted on `TRACKLINK_V3` hardware with locomotive `.pcm` sounds, `ConfigParser` SHALL load all sounds into PSRAM with zero transient memory allocation and stream smooth, glitch-free 22,050 Hz audio through the MAX98357A I2S DAC.

#### Scenario: On-device boot and playback
- **WHEN** the `TRACKLINK_V3` board boots with the locomotive filesystem
- **THEN** boot logs report successful `.pcm` sound ingestion in under 50ms, Free PSRAM is logged without fragmentation, and locomotive throttle/horn audio plays cleanly through the speaker
