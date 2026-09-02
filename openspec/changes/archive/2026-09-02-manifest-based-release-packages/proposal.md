## Why

Distributing loose `.bin` files (`.factory.bin` and `.ota.bin`) leaves flash offsets ambiguous for client applications and creates high failure risk if an OTA binary is accidentally flashed at offset `0x0000`. Adopting an industry-standard manifest-based ZIP package (`RC_Engine-<version>-<chip>-<board>.zip`) containing `manifest.json` along with segmented binaries (`bootloader.bin`, `partitions.bin`, `otadata.bin`, and `app.bin`) allows flash clients (like the RadioKit app flasher) and OTA engines to dynamically inspect target offsets and payload roles without hardcoded assumptions or loose file management.

## What Changes

- **Manifest-driven Packaging**: Update [`scripts/package_release.py`](file:///home/sun/Apps/RCKIT/RC_brain/scripts/package_release.py) to package each board environment into `RC_Engine-<version>-<chip>-<board>.zip`.
- **Standardized `manifest.json`**: Generate a structured `manifest.json` in each archive with chip family, board name, flash parameters, and a `parts` array listing each component (`bootloader.bin`, `partitions.bin`, `otadata.bin`, `app.bin`) with its offset (hex and decimal), file size, and SHA-256 hash.
- **Segment Extraction**: Extract `bootloader.bin`, `partitions.bin`, `firmware.bin` (as `app.bin`), and `boot_app0.bin` (as `otadata.bin`) into the package archive.
- **GitHub Actions Release**: Ensure `.github/workflows/release.yml` publishes these `.zip` packages as release assets.

## Capabilities

### New Capabilities
<!-- None -->

### Modified Capabilities
- `automated-github-releases`: Replace loose binary distribution requirements with standardized manifest-based ZIP packages containing `manifest.json` and segmented firmware binaries.

## Impact

- **Build / Packaging Tooling**: [`scripts/package_release.py`](file:///home/sun/Apps/RCKIT/RC_brain/scripts/package_release.py) updated to construct `manifest.json` and generate `.zip` archives.
- **Release Assets**: GitHub Release artifacts will now consist of 1 `.zip` per board environment instead of loose `.factory.bin` / `.ota.bin` files.
- **Flash / OTA Clients**: RadioKit Flasher tab and OTA updater can parse `manifest.json` to flash segmented binaries at explicit offsets or stream `app.bin` for OTA.
