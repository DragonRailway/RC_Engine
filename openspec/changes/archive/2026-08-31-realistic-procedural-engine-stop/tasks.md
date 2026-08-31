## 1. Configuration & Parameter Support

- [x] 1.1 Add `uint16_t stopDuration` to `EngineConfig` in `lib/SoundEngine/src/RcEngineSound.h`
- [x] 1.2 Update `ConfigParser::parseEngine()` in `common/ConfigParser.cpp` to set type-driven defaults (1400 Truck, 2800 Loco, 1800 Excavator) and parse optional `"stop_duration"`

## 2. Procedural Engine Shutdown Synthesis

- [x] 2.1 Implement `stopStartMillis`, `stopDurationMs`, continuous inertial pitch decay ($1.0 \to 0.18$), smooth volume envelope, and instant parking brake hand-off in `lib/SoundEngine/src/RcEngineSound.h` and `RcEngineSound.cpp`
- [x] 2.2 Update knock calculation in `RcEngineSound.cpp` to remain active during `STOPPING`, naturally stretching stroke cadence and fading volume
- [x] 2.3 Update turbo whistle volume to rapidly fade out over the first 50% of stop duration

## 3. Verification & Deployment

- [x] 3.1 Run host DSP, host VC, and config validation test suites (`host_dsp_test.py`, `host_vc_test.py`, `validate_configs.py`)
- [x] 3.2 Build and flash firmware to `TRACKLINK_V3` (`pio run -e TRACKLINK_V3 -t upload`)
- [x] 3.3 Verify realistic procedural engine stop behavior over serial
