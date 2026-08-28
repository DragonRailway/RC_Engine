## Context

`RC_Engine` supports multiple ESP32-S3 boards (`TRACKLINK_V3`, `MIKRO_V2`, and `GTRACK`) and maintains a rich library of vehicle soundpacks in `configs/`. Distributing new versions currently requires manual flashing from source. An automated CI/CD pipeline on GitHub Actions will build all environments on tag push or manual trigger, package merged `.factory.bin` and `.bin` images, and bundle the `configs/` directory into a downloadable zip file.

## Goals / Non-Goals

**Goals:**
- Add `[env:GTRACK]` to `platformio.ini` alongside existing boards and create `hardware-GTRACK.json`.
- Implement `.github/workflows/release.yml` with dual triggers (`push: tags: ['v*']` and `workflow_dispatch`).
- Dynamically discover and compile all declared environments in `platformio.ini`.
- Package both merged factory images (`0x0000`) and OTA binaries (`0x10000`) per board.
- Generate a clean `RC_Engine-configs.zip` containing `configs/`.
- Provide a standalone script `scripts/package_release.py` for identical local packaging.

**Non-Goals:**
- Automatic Over-The-Air deployment to connected devices (release assets are downloaded/flashed by users).

## Decisions

### Decision 1: GitHub Actions Workflow Triggers
Configure `.github/workflows/release.yml` with:
```yaml
on:
  push:
    tags:
      - 'v*'
  workflow_dispatch:
    inputs:
      tag_name:
        description: 'Release Tag (e.g. v1.0.0)'
        required: true
        default: 'v1.0.0'
```
- **Rationale**: Supports standard Git tag workflows while allowing maintainers to generate releases on-demand directly from the GitHub web interface.

### Decision 2: Environment Discovery
Instead of hardcoding board names in the CI script, use Python to parse `platformio.ini` (`[env:<name>]`) and build all boards dynamically.
- **Rationale**: Adding future board targets (e.g. `TRACKLINK_V4`) automatically includes them in GitHub releases without modifying CI yaml files.

### Decision 3: Factory + OTA Binary Format
For each board:
- `RC_Engine-<BOARD>-factory.bin` generated from `firmware.factory.bin` (contains bootloader, partition table, boot app0, and firmware for single-file web/USB flashing).
- `RC_Engine-<BOARD>-ota.bin` generated from `firmware.bin` (application partition only for RadioKit / OTA updates).
- `RC_Engine-configs.zip` (compressed `configs/` directory).

## Risks / Trade-offs

- **[Risk] GitHub Actions build time** → Mitigation: Cache PlatformIO packages and pip dependencies with `actions/cache` across workflow runs.
- **[Risk] SCons script dependencies in CI** → Mitigation: `scripts/fetch_radiokit.py` runs automatically before build on GitHub runners, pulling RadioKit seamlessly.
