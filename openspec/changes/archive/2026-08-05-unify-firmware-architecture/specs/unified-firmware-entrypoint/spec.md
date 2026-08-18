# Unified Firmware Entrypoint

## ADDED Requirements

### Requirement: Unified Main Entrypoint
The firmware SHALL have a single main entrypoint at `src/main.cpp` that initializes LittleFS, parses hardware and vehicle configurations, and starts audio and control services for any supported vehicle type.

#### Scenario: Firmware startup
- **WHEN** the ESP32 boots with `src/main.cpp`
- **THEN** LittleFS is mounted, `/hardware-config.json` and `/vehicle-config.json` are loaded, and hardware peripherals and sound engine are initialized

#### Scenario: Single PlatformIO environment
- **WHEN** building the project with `pio run`
- **THEN** PlatformIO builds the unified firmware using `[env:TRACKLINK_V3]` targeting `src/main.cpp`

### Requirement: Boot Failure Safety
The firmware SHALL fail safely and halt operation if LittleFS fails to mount or required configuration files cannot be parsed.

#### Scenario: Missing configuration file
- **WHEN** `/vehicle-config.json` or `/hardware-config.json` is missing or corrupted at boot
- **THEN** the firmware logs a fatal error over Serial at 2,000,000 baud and halts further execution without enabling PWM or audio outputs
