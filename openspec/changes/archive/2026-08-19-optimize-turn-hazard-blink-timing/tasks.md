# Tasks: Optimize Turn and Hazard Blink Timing

## 1. Config Defaults and Hardware Configs

- [x] 1.1 Update `common/Config.h` default `intervalOn = 300` and `intervalOff = 300` in `struct LightGroup`
- [x] 1.2 Update `common/ConfigParser.h` fallback defaults to 300ms for `turnLight.intervalOn` and `turnLight.intervalOff`
- [x] 1.3 Update `configs/hardware_configs/hardware-MIKRO_V2-truck.json`, `hardware-MIKRO_V2-skid.json`, and `hardware-TRACKLINK_V3-locomotive.json` `turn_light` interval parameters to 300ms

## 2. Verification and Testing

- [x] 2.1 Run host DSP tests (`python3 scripts/host_dsp_test.py`) and host vehicle controller tests (`python3 scripts/host_vc_test.py`)
- [x] 2.2 Build firmware for `MIKRO_V2` and `TRACKLINK_V3` (`pio run`)
- [x] 2.3 Deploy updated LittleFS filesystem bundle to MIKRO board and verify live on-device 300ms hazard blinking
