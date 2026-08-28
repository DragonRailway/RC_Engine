#!/usr/bin/env python3
"""Packaging utility to build all PlatformIO board environments and generate release assets."""

import argparse
import configparser
import os
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
PLATFORMIO_INI = REPO_ROOT / "platformio.ini"
CONFIGS_DIR = REPO_ROOT / "configs"
DEFAULT_DIST_DIR = REPO_ROOT / "dist"


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


def package_configs_zip(output_path):
    print(f"[Package] Zipping configs to {output_path}...")
    if output_path.exists():
        output_path.unlink()

    with zipfile.ZipFile(output_path, "w", zipfile.ZIP_DEFLATED) as zipf:
        for root, dirs, files in os.walk(CONFIGS_DIR):
            # Ignore hidden dirs or pycache
            dirs[:] = [d for d in dirs if not d.startswith(".") and d != "__pycache__"]
            for file in files:
                if file.startswith(".") or file.endswith("~") or file.endswith(".pyc"):
                    continue
                file_path = Path(root) / file
                arcname = file_path.relative_to(REPO_ROOT)
                zipf.write(file_path, arcname)
    print(f"[Package] Created configs archive: {output_path.name} ({output_path.stat().st_size / 1024:.1f} KB)")


def get_chip_for_env(env_name):
    config = configparser.ConfigParser()
    config.read(PLATFORMIO_INI)
    section = f"env:{env_name}"
    board = config.get(section, "board", fallback="esp32-s3-devkitc-1").lower()
    if "esp32-s3" in board or "esp32s3" in board:
        return "esp32s3"
    elif "esp32-c3" in board or "esp32c3" in board:
        return "esp32c3"
    elif "esp32-c6" in board or "esp32c6" in board:
        return "esp32c6"
    elif "esp32-s2" in board or "esp32s2" in board:
        return "esp32s2"
    elif "esp32-h2" in board or "esp32h2" in board:
        return "esp32h2"
    return "esp32"


def package_release(target_envs=None, dist_dir=DEFAULT_DIST_DIR, skip_build=False, version="v1.0.0"):
    dist_dir.mkdir(parents=True, exist_ok=True)
    all_envs = get_all_environments()
    envs_to_build = target_envs or all_envs
    ver_str = version if version.startswith("v") else f"v{version}"

    print(f"[Package] Version: {ver_str}")
    print(f"[Package] Target environments: {', '.join(envs_to_build)}")

    packaged_files = []

    # 1. Build and package firmware binaries per environment
    for env in envs_to_build:
        if not skip_build:
            build_environment(env)

        build_dir = REPO_ROOT / ".pio" / "build" / env
        factory_src = build_dir / "firmware.factory.bin"
        ota_src = build_dir / "firmware.bin"

        if not factory_src.exists() or not ota_src.exists():
            raise FileNotFoundError(f"Binary outputs missing in {build_dir}. Ensure build succeeded.")

        chip = get_chip_for_env(env)
        factory_dst = dist_dir / f"RC_Engine-{ver_str}-{chip}-{env}.bin"
        ota_dst = dist_dir / f"RC_Engine-{ver_str}-{chip}-{env}-ota.bin"

        shutil.copy2(factory_src, factory_dst)
        shutil.copy2(ota_src, ota_dst)

        packaged_files.extend([factory_dst, ota_dst])
        print(f"[Package] Copied {factory_dst.name} ({factory_dst.stat().st_size / 1024:.1f} KB)")
        print(f"[Package] Copied {ota_dst.name} ({ota_dst.stat().st_size / 1024:.1f} KB)")

    # 2. Package configs zip
    configs_zip = dist_dir / f"RC_Engine-{ver_str}-configs.zip"
    package_configs_zip(configs_zip)
    packaged_files.append(configs_zip)

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
