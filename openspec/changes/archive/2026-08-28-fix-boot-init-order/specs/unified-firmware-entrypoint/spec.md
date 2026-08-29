## MODIFIED Requirements

### Requirement: Unified Main Entrypoint
The firmware SHALL have a single main entrypoint at `src/main.cpp` that initializes LittleFS, parses hardware and vehicle configurations, and starts audio and control services for any supported vehicle type. LittleFS SHALL be mounted through `RKFs::begin()` (not directly via `LittleFS.begin()`) so that the RadioKit filesystem mount state is tracked centrally. In the RadioKit init sequence, `enableFS()` SHALL be called before `startBLE()` to ensure the filesystem is ready before the BLE stack starts.

#### Scenario: Firmware startup
- **WHEN** the ESP32 boots with `src/main.cpp`
- **THEN** LittleFS is mounted via `RKFs::begin()` in `ConfigParser::begin()`, the board hardware config (`/hardware-MIKRO_V2.json` or `/hardware-TRACKLINK_V3.json`) and `/vehicle-config.json` are loaded, and hardware peripherals and sound engine are initialized

#### Scenario: RadioKit init order
- **WHEN** `initRadioKit()` runs during boot
- **THEN** `RadioKit.enableFS()` is called before `RadioKit.startBLE()`, ensuring the filesystem is mounted before BLE advertising begins

#### Scenario: Single PlatformIO environment
- **WHEN** building the project with `pio run`
- **THEN** PlatformIO builds the unified firmware using `[env:TRACKLINK_V3]` targeting `src/main.cpp`
