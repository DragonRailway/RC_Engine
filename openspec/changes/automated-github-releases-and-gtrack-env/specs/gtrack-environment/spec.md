## ADDED Requirements

### Requirement: G-Track PlatformIO Environment
The build configuration `platformio.ini` SHALL declare `[env:GTRACK]` with ESP32-S3 board configurations, LittleFS 4MB partition table, PSRAM compiler flags, `-D GTRACK`, and appropriate include paths matching `TRACKLINK_V3` and `MIKRO_V2`.

#### Scenario: Compiling GTRACK environment
- **WHEN** running `pio run -e GTRACK`
- **THEN** the firmware compiles successfully with `GTRACK` pinouts and defines enabled

### Requirement: G-Track Hardware Configuration Resolution
The firmware in `src/main.cpp` SHALL map the `GTRACK` build define to load `/hardware-GTRACK.json` at boot and set the fallback device name to `"GTRACK"`.

#### Scenario: Boot with GTRACK define
- **WHEN** the firmware is compiled for `GTRACK`
- **THEN** `HW_CONFIG_PATH` resolves to `/hardware-GTRACK.json` and default board name is `"GTRACK"`

### Requirement: Default G-Track Hardware Profile
The repository SHALL provide `configs/hardware_configs/hardware-GTRACK.json` defining the physical pin layout and capabilities of the G-Track board.

#### Scenario: Hardware config existence
- **WHEN** reading `configs/hardware_configs/hardware-GTRACK.json`
- **THEN** the config validates against `configs/schemas/hardware_config.schema.json` with G-Track pin assignments
