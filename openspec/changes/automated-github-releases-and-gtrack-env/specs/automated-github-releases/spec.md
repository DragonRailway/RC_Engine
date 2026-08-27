## ADDED Requirements

### Requirement: Automated Multi-Board Release Workflow
The repository SHALL include a GitHub Actions workflow (`.github/workflows/release.yml`) that triggers upon pushing a tag matching `v*` or via manual `workflow_dispatch`, automatically builds all environments defined in `platformio.ini`, archives the `configs/` directory, and publishes the compiled assets to a GitHub Release.

#### Scenario: Triggered on version tag
- **WHEN** a tag such as `v1.0.0` is pushed to GitHub
- **THEN** the workflow compiles all declared PlatformIO environments, builds `RC_Engine-configs.zip`, and creates a GitHub Release with tag `v1.0.0` containing all artifacts

#### Scenario: Triggered via manual workflow dispatch
- **WHEN** a user triggers the release workflow manually in GitHub Actions with a release tag parameter
- **THEN** the workflow compiles all board environments, packages assets, and creates or updates the target GitHub Release

### Requirement: Standardized Firmware Binary Artifacts
The release packaging SHALL export both a merged factory binary (`RC_Engine-<BOARD>-factory.bin` for 0x0000 flashing) and an application binary (`RC_Engine-<BOARD>-ota.bin` for OTA updates) for every environment declared in `platformio.ini`.

#### Scenario: Factory and OTA binaries generated
- **WHEN** building environment `TRACKLINK_V3`
- **THEN** the release packaging produces `RC_Engine-TRACKLINK_V3-factory.bin` from `firmware.factory.bin` and `RC_Engine-TRACKLINK_V3-ota.bin` from `firmware.bin`

### Requirement: Standalone Configs Archive
The release workflow and packaging script SHALL generate a clean zip archive `RC_Engine-configs.zip` containing the entire `configs/` folder (hardware configurations, vehicle configurations, and sound sets), excluding transient files and OS metadata.

#### Scenario: Configs archive packaging
- **WHEN** the release packaging step executes
- **THEN** `RC_Engine-configs.zip` is created containing `hardware_configs/`, `vehicle_configs/`, and `schemas/`
