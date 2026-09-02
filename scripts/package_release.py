#!/usr/bin/env python3
"""Packaging utility to build all PlatformIO board environments and generate manifest-based release ZIP packages."""

import argparse
import configparser
import hashlib
import json
import os
import shutil
import subprocess
import sys
import zipfile
from datetime import datetime, timezone
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
PLATFORMIO_INI = REPO_ROOT / "platformio.ini"
DEFAULT_DIST_DIR = REPO_ROOT / "dist"

# Standard ESP32 otadata (boot_app0) initial buffer (8192 bytes, active slot = ota_0)
DEFAULT_BOOT_APP0_BYTES = (
    bytes([1, 0, 0, 0] + [0xFF] * 24 + [154, 152, 67, 71]) + (b"\xFF" * (8192 - 32))
)


def get_all_environments():
    config = configparser.ConfigParser()
    config.read(PLATFORMIO_INI)
    envs = []
    for section in config.sections():
        if section.startswith("env:") and section != "env":
            envs.append(section.split("env:", 1)[1])
    return envs


def build_environment(env_name):
    print(f"\n[Build] Compiling environment: {env_name}...")
    cmd = ["pio", "run", "-e", env_name]
    subprocess.run(cmd, cwd=REPO_ROOT, check=True)


def get_board_flash_meta(env_name):
    config = configparser.ConfigParser()
    config.read(PLATFORMIO_INI)
    sec = f"env:{env_name}"

    board = config.get(sec, "board", fallback="esp32-s3-devkitc-1").lower()
    if "esp32-s3" in board or "esp32s3" in board:
        chip_family = "ESP32-S3"
        chip_tag = "esp32s3"
        bootloader_offset_hex = "0x0000"
        bootloader_offset_dec = 0
    elif "esp32-c3" in board or "esp32c3" in board:
        chip_family = "ESP32-C3"
        chip_tag = "esp32c3"
        bootloader_offset_hex = "0x0000"
        bootloader_offset_dec = 0
    elif "esp32-c6" in board or "esp32c6" in board:
        chip_family = "ESP32-C6"
        chip_tag = "esp32c6"
        bootloader_offset_hex = "0x0000"
        bootloader_offset_dec = 0
    elif "esp32-s2" in board or "esp32s2" in board:
        chip_family = "ESP32-S2"
        chip_tag = "esp32s2"
        bootloader_offset_hex = "0x0000"
        bootloader_offset_dec = 0
    elif "esp32-h2" in board or "esp32h2" in board:
        chip_family = "ESP32-H2"
        chip_tag = "esp32h2"
        bootloader_offset_hex = "0x0000"
        bootloader_offset_dec = 0
    else:
        chip_family = "ESP32"
        chip_tag = "esp32"
        bootloader_offset_hex = "0x1000"
        bootloader_offset_dec = 4096

    # Flash properties with fallback to [env] section
    flash_size = config.get(sec, "board_build.flash_size", fallback=None) or config.get("env", "board_build.flash_size", fallback="4MB")
    flash_mode = config.get(sec, "board_build.flash_mode", fallback=None) or config.get("env", "board_build.flash_mode", fallback="qio")
    f_flash_raw = config.get(sec, "board_build.f_flash", fallback=None) or config.get("env", "board_build.f_flash", fallback="80000000L")
    flash_freq = "80m" if "80" in f_flash_raw else "40m"

    return {
        "chip_family": chip_family,
        "chip_tag": chip_tag,
        "bootloader_offset_hex": bootloader_offset_hex,
        "bootloader_offset_dec": bootloader_offset_dec,
        "flash_size": flash_size,
        "flash_mode": flash_mode,
        "flash_freq": flash_freq,
    }


def resolve_boot_app0_bytes():
    """Finds boot_app0.bin in PlatformIO framework tools or returns standard bytes."""
    search_dirs = [
        Path.home() / ".platformio" / "packages",
        Path.home() / "sandbox" / "fedora" / ".platformio" / "packages",
    ]
    for base in search_dirs:
        if base.exists():
            matches = list(base.glob("**/boot_app0.bin"))
            if matches:
                return matches[0].read_bytes()
    return DEFAULT_BOOT_APP0_BYTES


def calculate_sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def package_release(target_envs=None, dist_dir=DEFAULT_DIST_DIR, skip_build=False, version="v1.0.0"):
    dist_dir.mkdir(parents=True, exist_ok=True)
    all_envs = get_all_environments()
    envs_to_build = target_envs or all_envs
    ver_str = version if version.startswith("v") else f"v{version}"

    print(f"[Package] Version: {ver_str}")
    print(f"[Package] Target environments: {', '.join(envs_to_build)}")

    otadata_bytes = resolve_boot_app0_bytes()
    packaged_files = []

    # Build and package manifest-based ZIP packages per environment
    for env in envs_to_build:
        if not skip_build:
            build_environment(env)

        build_dir = REPO_ROOT / ".pio" / "build" / env
        bootloader_path = build_dir / "bootloader.bin"
        partitions_path = build_dir / "partitions.bin"
        app_path = build_dir / "firmware.bin"

        if not bootloader_path.exists() or not partitions_path.exists() or not app_path.exists():
            raise FileNotFoundError(f"Binary outputs missing in {build_dir}. Ensure build succeeded.")

        bootloader_bytes = bootloader_path.read_bytes()
        partitions_bytes = partitions_path.read_bytes()
        app_bytes = app_path.read_bytes()

        flash_meta = get_board_flash_meta(env)

        clean_ver = ver_str.lstrip("v")
        manifest = {
            "manifest_version": 1,
            "name": "RC_Engine",
            "version": clean_ver,
            "board": env,
            "chipFamily": flash_meta["chip_family"],
            "chip_family": flash_meta["chip_family"],
            "flash_mode": flash_meta["flash_mode"],
            "flash_size": flash_meta["flash_size"],
            "flash_freq": flash_meta["flash_freq"],
            "builds": [
                {
                    "chipFamily": flash_meta["chip_family"],
                    "parts": [
                        {
                            "path": "bootloader.bin",
                            "offset": flash_meta["bootloader_offset_dec"],
                        },
                        {
                            "path": "partitions.bin",
                            "offset": 32768,
                        },
                        {
                            "path": "otadata.bin",
                            "offset": 57344,
                        },
                        {
                            "path": "app.bin",
                            "offset": 65536,
                        },
                    ],
                }
            ],
            "created_at": datetime.now(timezone.utc).isoformat(),
            "parts": [
                {
                    "name": "bootloader",
                    "path": "bootloader.bin",
                    "offset": flash_meta["bootloader_offset_hex"],
                    "offset_dec": flash_meta["bootloader_offset_dec"],
                    "size": len(bootloader_bytes),
                    "sha256": calculate_sha256(bootloader_bytes),
                },
                {
                    "name": "partitions",
                    "path": "partitions.bin",
                    "offset": "0x8000",
                    "offset_dec": 32768,
                    "size": len(partitions_bytes),
                    "sha256": calculate_sha256(partitions_bytes),
                },
                {
                    "name": "otadata",
                    "path": "otadata.bin",
                    "offset": "0xE000",
                    "offset_dec": 57344,
                    "size": len(otadata_bytes),
                    "sha256": calculate_sha256(otadata_bytes),
                },
                {
                    "name": "app",
                    "path": "app.bin",
                    "offset": "0x10000",
                    "offset_dec": 65536,
                    "size": len(app_bytes),
                    "sha256": calculate_sha256(app_bytes),
                    "role": "app",
                    "ota_supported": True,
                },
            ],
        }

        manifest_json_bytes = json.dumps(manifest, indent=2).encode("utf-8")

        zip_dst = dist_dir / f"RC_Engine-{ver_str}-{flash_meta['chip_tag']}-{env}.zip"
        if zip_dst.exists():
            zip_dst.unlink()

        with zipfile.ZipFile(zip_dst, "w", zipfile.ZIP_DEFLATED) as zipf:
            zipf.writestr("manifest.json", manifest_json_bytes)
            zipf.writestr("bootloader.bin", bootloader_bytes)
            zipf.writestr("partitions.bin", partitions_bytes)
            zipf.writestr("otadata.bin", otadata_bytes)
            zipf.writestr("app.bin", app_bytes)

        packaged_files.append(zip_dst)
        print(f"[Package] Created {zip_dst.name} ({zip_dst.stat().st_size / 1024:.1f} KB)")

    print("\n=======================================================")
    print(f"🎉 Successfully packaged {len(packaged_files)} release assets in {dist_dir}:")
    for f in packaged_files:
        print(f"  • {f.name} ({f.stat().st_size / 1024:.1f} KB)")
    print("=======================================================\n")
    return packaged_files


def main():
    parser = argparse.ArgumentParser(description="Package RC_Engine release artifacts")
    parser.add_argument("--env", action="append", help="Target specific environment(s)")
    parser.add_argument("--version", default="v1.0.0", help="Release version tag (e.g. v1.0.0)")
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_DIST_DIR, help="Destination directory for artifacts")
    parser.add_argument("--skip-build", action="store_true", help="Skip compilation, package existing binaries")
    args = parser.parse_args()

    package_release(target_envs=args.env, dist_dir=args.output_dir, skip_build=args.skip_build, version=args.version)


if __name__ == "__main__":
    main()
