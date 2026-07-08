# RC Brain - AGENTS.md

## Project Overview
ESP32-based RC vehicle controller (TRACKLINK_V3 board) with configurable hardware, sound engine, and vehicle profiles. Built with PlatformIO, Arduino framework, and LittleFS for config storage.

## Key Components
- **src/main.cpp** - Entry point, runs config loader test
- **src/boards/TRACKLINK_V3.h** - Pin definitions for Elecrow TrackLink V3 board
- **src/ConfigLoader.h** - LittleFS + ArduinoJson config loading utility
- **src/example_config/** - Hardware & vehicle JSON config examples
- **src/sounds/** - Sound profile JSON files
- **lib/RcEngineSound/** - Sound engine library with vehicle profiles

## Build Commands
```bash
# Build project
pio run

# Build specific environment
pio run -e TRACKLINK_V3

# Clean build
pio run -t clean

# Upload to device
pio run -t upload

# Monitor serial (2000000 baud)
pio device monitor
```

## Test Commands
```bash
# Run unit tests (PlatformIO test runner)
pio test

# Run tests for specific environment
pio test -e TRACKLINK_V3
```

## Project Structure
```
RC_brain/
├── platformio.ini           # PlatformIO config (ESP32-S3, Arduino, ArduinoJson)
├── convert_sounds.sh        # Converts C header sound arrays to JSON
├── src/
│   ├── main.cpp             # Config loader test app
│   ├── boards/TRACKLINK_V3.h # Pin mappings (H-bridge, LEDs, I2S, power)
│   ├── ConfigLoader.h       # LittleFS + JSON config utilities
│   ├── example_config/      # hardware-config.json, vehicle-config.json
│   └── sounds/              # Sound profile JSONs (idle, rev, horn, etc.)
├── lib/RcEngineSound/       # Sound engine library
│   ├── vehicles/            # Vehicle sound profiles (ScaniaV8, etc.)
│   └── sounds/              # Raw sound data headers
└── test/                    # PlatformIO unit tests (empty)
```

## Configuration
- **Hardware config** (`hardware-*.json`): Pin assignments, motor/servo/light configs
- **Vehicle config** (`vehicle-*.json`): Engine params, transmission, sound volumes
- **Sound files** (`sounds/*.json`): Sample rate, count, PCM data arrays

## Dependencies
- ESP32 Arduino core (platform-espressif32)
- ArduinoJson ^7.0.4
- ESP32_PWM_Fusion (local symlink)
- LittleFS (built-in)

## Board Defines (platformio.ini)
- `TRACKLINK_V3` - Board identifier
- `BOOT=0` - Boot button pin
- `VSCALE=1.8`, `VOFFSET=-0.2` - Voltage sensing calibration
- `LED_FREQ=8000`, `LED_RES=10` - LED PWM
- `DRV_FREQ=24000`, `DRV_RES=8` - Motor driver PWM

## Sound Conversion
Run `./convert_sounds.sh` to convert C header sound arrays in `lib/RcEngineSound/src/vehicles/sounds/` to prefixed JSON files in `lib/RcEngineSound/src/vehicles/sounds/json/`.

## Common Tasks
1. **Add new vehicle**: Create header in `lib/RcEngineSound/vehicles/`, add sounds to `lib/RcEngineSound/sounds/`, run convert script
2. **Modify pin mapping**: Edit `src/boards/TRACKLINK_V3.h`
3. **Update config schema**: Modify example configs in `src/example_config/`
4. **Deploy configs**: Copy JSON files to device LittleFS (via PlatformIO filesystem upload)