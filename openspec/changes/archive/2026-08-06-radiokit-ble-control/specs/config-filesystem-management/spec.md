# Config Filesystem Management

## ADDED Requirements

### Requirement: Remote filesystem access
The firmware SHALL enable the RadioKit filesystem feature (`RK_ENABLE_FS` defined and `RadioKit.enableFS()` called) so the companion app can browse, read, write, upload, and delete files on LittleFS — including `/hardware-config.json`, `/vehicle-config.json`, and `/sounds/*.json`.

#### Scenario: App lists the filesystem
- **WHEN** the app requests a directory listing
- **THEN** the firmware returns the LittleFS directory contents including config and sound files

#### Scenario: App uploads a sound file
- **WHEN** the app uploads a new sound JSON to `/sounds/`
- **THEN** the file is stored on LittleFS and is available to the sound loader on the next load

### Requirement: Config reload
The firmware SHALL detect changes to the config files on LittleFS and reload hardware and vehicle configuration at runtime: re-parse configs, re-initialize hardware outputs, and re-apply the sound engine config without rebooting.

#### Scenario: Hardware config edited in app
- **WHEN** `/hardware-config.json` is modified via the app filesystem manager
- **THEN** the firmware re-parses the file and applies the new pin/duty/endpoint settings via hot reload

#### Scenario: Vehicle config edited in app
- **WHEN** `/vehicle-config.json` is modified via the app filesystem manager
- **THEN** the firmware re-parses the file and re-applies engine, transmission, and sound volume settings to the sound engine

#### Scenario: Reload preserves boot failure safety
- **WHEN** a reload is attempted but the config file is missing or corrupt
- **THEN** the firmware logs an error, keeps the last known-good configuration active, and does not halt the control loop
