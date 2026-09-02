## Context

The current release build script produces loose firmware binaries. However, flashing ESP32-S3 boards requires precise knowledge of partition offsets (bootloader at 0x0000, partitions at 0x8000, otadata at 0xE000, app at 0x10000). When clients or users manually pick files, flashing an OTA binary at offset 0x0 overwrites the bootloader and corrupts flash. Following standard IoT patterns (such as ESP Web Tools and ESPHome), packaging all components with a `manifest.json` into a standard `.zip` file enables deterministic, multi-segment flashing and clean OTA extraction.

## Goals / Non-Goals

**Goals:**
- Package each compiled board environment into a self-contained `RC_Engine-<version>-<chip>-<board>.zip`.
- Include standard `manifest.json` describing target hardware (`chip_family`, `board`, `flash_mode`, `flash_size`, `flash_freq`) and declaring `parts` with offsets (hex and decimal), sizes, and SHA-256 digests.
- Include all segmented binary components: `bootloader.bin`, `partitions.bin`, `otadata.bin` (standard `boot_app0.bin`), and `app.bin` (`firmware.bin`).
- Integrate into `.github/workflows/release.yml` and [`scripts/package_release.py`](file:///home/sun/Apps/RCKIT/RC_brain/scripts/package_release.py).

**Non-Goals:**
- Packaging dynamic LittleFS vehicle sound bundles into the firmware package (LittleFS bundles remain separate per vehicle configuration).
- Modifying on-device C++ runtime code (the changes are restricted to release build tools and packaging specs).

## Decisions

### 1. Package Format: Standard `.zip` with Root Manifest
- **Decision**: Produce a `.zip` archive per environment named `RC_Engine-<version>-<chip>-<board>.zip` containing `manifest.json`, `bootloader.bin`, `partitions.bin`, `otadata.bin`, and `app.bin`.
- **Rationale**: `.zip` is universally supported across Dart/Flutter (`archive` package), Python, Web, and desktop environments without requiring proprietary unpackers.
- **Alternatives Considered**: `.rkpkg` custom extension (rejected to maintain standard file extension compatibility).

### 2. Multi-Segment Manifest Schema
- **Decision**: Follow ESP Web Tools conventions with dual-format offsets (`offset` as hex string like `"0x10000"` and `offset_dec` as integer like `65536`), along with SHA-256 hashes and explicit roles (`role: "app"`, `ota_supported: true`).
- **Rationale**: Gives both human-readable debug info and direct numeric offsets for serial flashers, while allowing OTA engines to identify the application binary unambiguously.

### 3. Sourcing `otadata.bin` (`boot_app0.bin`)
- **Decision**: In [`scripts/package_release.py`](file:///home/sun/Apps/RCKIT/RC_brain/scripts/package_release.py), locate `boot_app0.bin` dynamically from the PlatformIO Arduino-ESP32 framework packages or fallback to a standard static 8KB `boot_app0` template if compiling in standalone environments.
- **Rationale**: `boot_app0.bin` is a constant 8192-byte binary required to initialize the active OTA slot to `ota_0` (partition offset `0x10000`).

## Risks / Trade-offs

- **[Risk] Missing `boot_app0.bin` in CI or custom environments** → **Mitigation**: Script will search `~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin` and provide an embedded fallback generator if not found.
- **[Risk] Flasher app compatibility** → **Mitigation**: Providing explicit `offset` and `offset_dec` ensures compatibility across both string-based and int-based JSON parsers in Flutter / Web flashers.
