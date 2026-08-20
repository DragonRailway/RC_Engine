# Tasks: Optimize Hardware Light Fade

## 1. Hardware Config Updates

- [x] 1.1 Update `fade_duration_ms: 0` in `configs/hardware_configs/hardware-MIKRO_V2-truck.json`, `hardware-MIKRO_V2-skid.json`, and `hardware-TRACKLINK_V3-locomotive.json`
- [x] 1.2 Update default `fadeDurationMs = 0` in `common/Config.h` and fallback in `common/ConfigParser.h`

## 2. Testing and Verification

- [x] 2.1 Run host DSP and VC tests (`python3 scripts/host_dsp_test.py && python3 scripts/host_vc_test.py`)
  - Host VC tests: all 20/20 pass
  - Host DSP tests: all pass, zero glitches
  - Config validation: 4 hardware configs + 76 bundles validated
  - Builds: MIKRO_V2 and TRACKLINK_V3 compile cleanly (52.5% RAM, 50.5% Flash)
- [ ] 2.2 Flash LittleFS filesystem to MIKRO board and verify instant LED toggle
  - Note: requires physical hardware
