# RC Brain - AGENTS.md

## Project Overview
ESP32-based RC vehicle controller (TRACKLINK_V3 board) with configurable hardware, sound engine, and vehicle profiles. Built with PlatformIO, Arduino framework, and LittleFS for config storage.

## Key Components
- **src/main.cpp** - Entry point; loads configs, runs the control loop and RadioKit
- **common/** - Shared headers: `Config.h`/`ConfigParser.h` (LittleFS + ArduinoJson), `HardwareInit.h` (peripherals + light animation), `VehicleController.h` (drive/light/gear logic), `RADIOKIT.h` (generated RadioKit surface)
- **boards/** - Pin definitions per board (TRACKLINK_V3.h, etc.), generated into `boards/boards.h` by `scripts/gen_boards_header.py`
- **configs/** - The deployable config library (bundle model):
  - `configs/hardware_configs/hardware-<BOARD>.json` — one per board
  - `configs/vehicle_configs/<sound_set>/` — self-contained vehicle bundles (`vehicle.json` + `sounds/`)
  - `configs/vehicle_configs/common/<preset>/` — shared preset fallback sounds
- **lib/SoundEngine/** - Sound engine library; **lib/rk-arduino** - vendored RadioKit stack; **ESP32_EasyKit** - standalone sibling repo (`../ESP32_EasyKit`)
- **references/** - Legacy C-header sources (raw_sounds, raw_vehicles, upstream libs)
- **data/** - Gitignored scratch (never deployed; see FS deployment below)
- **scripts/build_fs.py** - The ONLY supported LittleFS flash path

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
├── platformio.ini            # PlatformIO config (ESP32-S3, pioarduino, Arduino, ArduinoJson)
├── partitions_ota_4MB.csv    # OTA partition table (spiffs/LittleFS @ 0x310000, 896 KB)
├── src/
│   └── main.cpp              # Entry point: config load, control loop, RadioKit
├── common/                   # Shared headers (Config, ConfigParser, HardwareInit, VehicleController)
├── boards/                   # Per-board pin definitions (+ generated boards.h)
├── configs/                  # Deployable config library (bundle model)
│   ├── hardware_configs/     # hardware-<BOARD>.json per board
│   └── vehicle_configs/      # <sound_set>/{vehicle.json, sounds/*.json} + common/<preset>/
├── lib/                      # SoundEngine, rk-arduino (vendored); ESP32_EasyKit via ../ESP32_EasyKit
├── references/               # Legacy raw C-header sources + upstream libs
├── data/                     # Gitignored scratch (never deployed)
├── scripts/                  # build_fs.py, validate_sounds.py, smoke/hot-reload/DSP tools
├── test/                     # Host test harnesses (host_vc, host_dsp), golden baselines
└── docs/                     # RadioKit API docs, audio debug tooling guide
```

## FS Deployment (config-bundles model)
The board's LittleFS holds exactly **one hardware config + one vehicle bundle**. The full `data/` tree is ~132 MB vs an 896 KB partition, so `pio run -t uploadfs` is **not supported** (blocked by a guardrail) — it would build from empty scratch and the board would boot FATAL.

```bash
# Deploy a board's filesystem: hardware config + vehicle bundle + preset fallbacks
python3 scripts/build_fs.py --board MIKRO_V2 --vehicle ScaniaV8
# Size report only
python3 scripts/build_fs.py --board MIKRO_V2 --vehicle ScaniaV8 --dry-run
```

Config-only tweaks on a live board still hot-reload over RadioKit FS upload (no flash needed).

## Configuration
- **Hardware config** (`configs/hardware_configs/hardware-*.json`): Pin assignments, motor/servo/light configs, optional `animation` block
- **Vehicle config** (`configs/vehicle_configs/<set>/vehicle.json`): Engine params, transmission, sound volumes; `sound_set` must equal its bundle dir name
- **Sound files** (`.../sounds/*.json`): Sample rate, count, PCM data arrays

## Config validation
- **Firmware**: `common/ConfigParser.h` logs `WARN:` lines for semantic config errors at boot and hot-reload (unknown keys, unrecognized `hardware` tokens, out-of-range values, unrecognized enums) — warn-and-continue, never halts.
- **Host**: `scripts/build_fs.py` validates the hardware config against `configs/schemas/hardware_config.schema.json` before staging (aborts flash on violations). `scripts/validate_configs.py` validates every config in the repo (CI-able).

## RadioKit Remote Access API
- **Where**: embedded in the RadioKit Flutter app on the Android phone (`10.0.0.6:7007`), reachable via `adb forward tcp:17007 tcp:7007` (direct LAN curl fails).
- **Docs**: `docs/radiokit-api/README.md` (index + workflows), `skills/*.md` (server skill guides), `api-schema.json`, `root.md`.
- **Design**: `docs/radiokit-rc-ui-design.json` — the RC_UI control surface; `src/RADIOKIT.h` is generated from it via `GET /api/designs/<id>/header`.
