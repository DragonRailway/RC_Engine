## MODIFIED Requirements

### Requirement: Automated Multi-Board Release Workflow
The repository SHALL include a GitHub Actions workflow (`.github/workflows/release.yml`) that triggers upon pushing a tag matching `v*` or via manual `workflow_dispatch`, automatically builds all environments defined in `platformio.ini`, and publishes the compiled firmware release packages to a GitHub Release.

#### Scenario: Triggered on version tag
- **WHEN** a tag such as `v1.0.0` is pushed to GitHub
- **THEN** the workflow compiles all declared PlatformIO environments and creates a GitHub Release with tag `v1.0.0` containing all packaged `.zip` artifacts

#### Scenario: Triggered via manual workflow dispatch
- **WHEN** a user triggers the release workflow manually in GitHub Actions with a release tag parameter
- **THEN** the workflow compiles all board environments, packages assets into release ZIPs, and creates or updates the target GitHub Release

### Requirement: Standardized Firmware Binary Artifacts
The release packaging SHALL export a manifest-based ZIP archive (`RC_Engine-<version>-<chip>-<BOARD>.zip`) for every environment declared in `platformio.ini`, containing `manifest.json` and all segmented binary parts (`bootloader.bin`, `partitions.bin`, `otadata.bin`, and `app.bin`).

#### Scenario: Manifest and segmented binaries generated
- **WHEN** building environment `TRACKLINK_V3` with version `v1.0.0`
- **THEN** the release packaging produces `RC_Engine-v1.0.0-esp32s3-TRACKLINK_V3.zip` containing `manifest.json`, `bootloader.bin`, `partitions.bin`, `otadata.bin`, and `app.bin`

#### Scenario: Manifest schema and offsets validation
- **WHEN** a release ZIP package is inspected
- **THEN** `manifest.json` contains valid chip family, board name, flash settings, and a `parts` list declaring decimal and hex flash offsets, sizes, and SHA-256 hashes for all included binary parts
