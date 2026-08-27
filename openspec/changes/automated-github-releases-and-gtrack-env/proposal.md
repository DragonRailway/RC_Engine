## Why

Users need ready-to-flash binary releases for all supported RC_Engine hardware boards (TRACKLINK_V3, MIKRO_V2, and G-Track) and a downloadable archive of the complete vehicle and hardware configs library without having to install PlatformIO or compile from source. We need an automated GitHub Actions release workflow that triggers on release tags (`v*`) and manual workflow dispatches, builds all environments in `platformio.ini`, packages firmware images (factory + OTA) and `configs/`, and creates GitHub releases.

## What Changes

- Add `[env:GTRACK]` to `platformio.ini` with appropriate ESP32-S3 flags and include paths.
- Update `src/main.cpp` to recognize `GTRACK` (`/hardware-GTRACK.json` and `"GTRACK"` default name).
- Add default hardware configuration `configs/hardware_configs/hardware-GTRACK.json`.
- Add `.github/workflows/release.yml` for automated multi-board builds, packaging (`RC_Engine-<BOARD>-factory.bin`, `RC_Engine-<BOARD>-ota.bin`, and `RC_Engine-configs.zip`), and publishing to GitHub Releases on tag push or manual workflow dispatch.
- Add `scripts/package_release.py` local packaging utility to dynamically inspect `platformio.ini`, compile all declared environments, and assemble the release assets locally.

## Capabilities

### New Capabilities
- `automated-github-releases`: Automated GitHub Actions release workflow and packaging utility that dynamically builds all environments defined in `platformio.ini`, generates combined factory binaries, OTA binaries, and a zipped `configs/` bundle, and publishes them to GitHub Releases on tag or manual trigger.
- `gtrack-environment`: Dedicated PlatformIO build environment, firmware defines, and default hardware configuration for the G-Track board.

### Modified Capabilities
<!-- None -->

## Impact

- **CI/CD**: Adds `.github/workflows/release.yml` running on GitHub Actions.
- **Hardware Targets**: `platformio.ini` now declares environments for `TRACKLINK_V3`, `MIKRO_V2`, and `GTRACK`.
- **Packaging**: Produces standalone `configs.zip` and board binaries on every release.
