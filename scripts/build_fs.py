#!/usr/bin/env python3
"""Manifest-driven LittleFS deployment for RC_brain (config-bundles model).

Assembles a board's LittleFS image from the configs/ library — exactly one
hardware config + one vehicle bundle (+ shared preset fallbacks) — and flashes
it to the spiffs partition. This is the ONLY supported FS-flash path; the
PlatformIO `uploadfs` target builds from the hardcoded `data/` tree and always
fails with LFS_ERR_NOSPC (the full repo tree is ~132 MB vs an 896 KB partition).

Layout mapping (repo -> board LittleFS):
  configs/hardware_configs/hardware-<BOARD>.json -> /hardware-<BOARD>.json
  configs/vehicle_configs/<V>/vehicle.json       -> /vehicle-config.json
  configs/vehicle_configs/<V>/sounds/<slot>.json -> /sounds/vehicles/<V>/<slot>.json
  configs/vehicle_configs/common/<preset>/...    -> /sounds/presets/<preset>/...
      (only for slots the vehicle bundle does NOT provide — tier-2 fallback)

The image is built with the same `littlefs` Python package (and identical
parameters) that the pioarduino espressif32 platform uses for `uploadfs`, so
the output is byte-for-byte compatible with what the firmware expects.

Usage:
  scripts/build_fs.py --board MIKRO_V2 --vehicle ScaniaV8            # build + flash
  scripts/build_fs.py --board MIKRO_V2 --vehicle ScaniaV8 --dry-run  # size report only
  scripts/build_fs.py --board MIKRO_V2 --vehicle ScaniaV8 --no-flash # build image only
"""
import argparse
import configparser
import csv
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
PARTITIONS_CSV = REPO / "partitions_ota_4MB.csv"
CONFIGS = REPO / "configs"
HW_DIR = CONFIGS / "hardware_configs"
VEHICLE_DIR = CONFIGS / "vehicle_configs"
COMMON_DIR = VEHICLE_DIR / "common"
STAGING_ROOT = REPO / ".pio" / "fs_staging"

# Parsed from partitions_ota_4MB.csv (data/spiffs row).
SPIFFS_TYPE = "spiffs"


def die(msg):
    print(f"ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def parse_partition(partitions_csv, subtype):
    """Return (offset, size) of the first data partition with the given subtype."""
    with open(partitions_csv) as f:
        for row in csv.reader(f):
            if not row or row[0].startswith("#"):
                continue
            name, ptype, stype, offset, size = [c.strip() for c in row[:5]]
            if ptype == "data" and stype == subtype:
                return int(offset, 0), int(size, 0)
    die(f"partition {subtype!r} not found in {partitions_csv}")


def find_pio_home():
    env = os.environ.get("PLATFORMIO_CORE_DIR")
    if env:
        p = Path(env).expanduser()
        if p.is_dir():
            return p
    p = Path.home() / ".platformio"
    if p.is_dir():
        return p
    die("PlatformIO core dir not found (set PLATFORMIO_CORE_DIR or run under HOME with ~/.platformio)")


def littlefs_build(penv_python, staging, out_bin, fs_size, block_size=4096):
    """Build the LittleFS image via the same littlefs-python the platform uses."""
    code = r"""
import json, sys
from pathlib import Path
from littlefs import LittleFS

staging = Path(sys.argv[1])
out = Path(sys.argv[2])
fs_size = int(sys.argv[3])
block_size = int(sys.argv[4])

fs = LittleFS(
    block_size=block_size,
    block_count=fs_size // block_size,
    read_size=1,
    prog_size=1,
    cache_size=block_size,
    lookahead_size=32,
    block_cycles=500,
    name_max=64,
    disk_version=(2 << 16) | 1,  # LittleFS v2.1
    mount=True,
)
for item in sorted(staging.rglob("*")):
    rel = item.relative_to(staging).as_posix()
    if item.is_dir():
        fs.makedirs(rel, exist_ok=True)
    else:
        fs.makedirs(rel.rpartition("/")[0], exist_ok=True) if "/" in rel else None
        with fs.open(rel, "wb") as dst:
            dst.write(item.read_bytes())
with open(out, "wb") as f:
    f.write(fs.context.buffer)
print(f"built {out} ({fs_size} bytes, {fs_size // block_size} blocks)")
"""
    subprocess.run([str(penv_python), "-c", code, str(staging), str(out_bin),
                    str(fs_size), str(block_size)], check=True)


def board_chip(board, pio_ini):
    """Resolve the ESP chip from the env's board in platformio.ini."""
    try:
        cp = configparser.ConfigParser()
        cp.read(pio_ini)
        section = f"env:{board}"
        if cp.has_section(section) and cp.has_option(section, "board"):
            b = cp.get(section, "board").replace("-", "")
            if "esp32s3" in b:
                return "esp32s3"
            if "esp32s2" in b:
                return "esp32s2"
            if "esp32c3" in b:
                return "esp32c3"
            if "esp32c6" in b:
                return "esp32c6"
            if "esp32h2" in b:
                return "esp32h2"
    except Exception:
        pass
    return "esp32"


def assemble(board, vehicle, pio_home, fs_size, dry_run=False, no_flash=False, port=None):
    hw_src = HW_DIR / f"hardware-{board}.json"
    if not hw_src.is_file():
        die(f"hardware config not found: {hw_src.relative_to(REPO)}")
    bundle = VEHICLE_DIR / vehicle
    vc_src = bundle / "vehicle.json"
    sounds_src = bundle / "sounds"
    if not vc_src.is_file():
        die(f"vehicle bundle not found: {bundle.relative_to(REPO)} (missing vehicle.json)")
    if not sounds_src.is_dir():
        die(f"vehicle bundle has no sounds/ dir: {bundle.relative_to(REPO)}")

    try:
        with open(vc_src) as f:
            vc = json.load(f)
        preset = (vc.get("vehicle") or {}).get("preset")
    except (json.JSONDecodeError, OSError) as e:
        die(f"cannot parse {vc_src.relative_to(REPO)}: {e}")

    staging = STAGING_ROOT / f"{board}-{vehicle}"
    if staging.exists():
        shutil.rmtree(staging)
    staging.mkdir(parents=True)

    # /hardware-<BOARD>.json
    shutil.copy2(hw_src, staging / hw_src.name)
    # /vehicle-config.json
    shutil.copy2(vc_src, staging / "vehicle-config.json")
    # /sounds/vehicles/<V>/<slot>.json
    (staging / "sounds" / "vehicles" / vehicle).mkdir(parents=True)
    for slot in sorted(sounds_src.glob("*.json")):
        shutil.copy2(slot, staging / "sounds" / "vehicles" / vehicle / slot.name)

    # Tier-2 preset fallbacks: only slots the vehicle bundle does NOT provide.
    # NOTE: never create the preset dir unless it gets at least one file — an
    # empty directory still costs a littlefs metadata pair, which can push a
    # near-full bundle (e.g. ScaniaV8) past the partition size.
    vehicle_slots = {s.name for s in sounds_src.glob("*.json")}
    if preset:
        preset_dir = COMMON_DIR / preset
        fallbacks = [s for s in sorted(preset_dir.glob("*.json")) if s.name not in vehicle_slots] if preset_dir.is_dir() else []
        if fallbacks:
            (staging / "sounds" / "presets" / preset).mkdir(parents=True)
            for slot in fallbacks:
                shutil.copy2(slot, staging / "sounds" / "presets" / preset / slot.name)

    total = sum(f.stat().st_size for f in staging.rglob("*") if f.is_file())
    print(f"Assembled {board}/{vehicle}: {total} bytes "
          f"({fs_size - total} free of {fs_size} partition)")
    if total > fs_size:
        die(f"assembled size {total} exceeds spiffs partition {fs_size} — "
            f"drop sounds or pick a smaller bundle")
    if dry_run:
        print("DRY RUN — no image built, no flash.")
        return

    # Build the image.
    penv_python = pio_home / "penv" / "bin" / "python"
    if not penv_python.is_file():
        die(f"PlatformIO penv python not found: {penv_python}")
    try:
        subprocess.run([str(penv_python), "-c", "import littlefs"], check=True,
                       capture_output=True)
    except subprocess.CalledProcessError:
        die("littlefs Python package missing from PlatformIO penv "
            f"({penv_python} -c 'import littlefs')")
    out_bin = STAGING_ROOT / f"{board}-{vehicle}.bin"
    littlefs_build(penv_python, staging, out_bin, fs_size)
    print(f"Image: {out_bin.relative_to(REPO)} ({out_bin.stat().st_size} bytes)")

    if no_flash:
        print("NO FLASH — image built only.")
        return

    # Flash via esptool at the partition offset.
    esptool = pio_home / "packages" / "tool-esptoolpy" / "esptool.py"
    if not esptool.is_file():
        esptool = pio_home / "tools" / "tool-esptoolpy" / "esptool.py"
    if not esptool.is_file():
        die(f"esptool not found under {pio_home}")
    offset, _ = parse_partition(PARTITIONS_CSV, SPIFFS_TYPE)
    chip = board_chip(board, REPO / "platformio.ini")
    port = port or os.environ.get("UPLOAD_PORT", "/dev/ttyACM0")
    cmd = [str(penv_python), str(esptool), "--chip", chip, "--port", port,
           "--baud", "921600", "write_flash", hex(offset), str(out_bin)]
    print("Flashing: " + " ".join(cmd))
    subprocess.run(cmd, check=True)
    print("Flash complete.")


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--board", required=True, help="hardware config id, e.g. MIKRO_V2")
    ap.add_argument("--vehicle", required=True, help="vehicle bundle dir name, e.g. ScaniaV8")
    ap.add_argument("--dry-run", action="store_true", help="print size report only")
    ap.add_argument("--no-flash", action="store_true", help="build image, don't flash")
    ap.add_argument("--port", default=None, help="serial port (default $UPLOAD_PORT or /dev/ttyACM0)")
    args = ap.parse_args()

    if not PARTITIONS_CSV.is_file():
        die(f"partitions file not found: {PARTITIONS_CSV}")
    offset, fs_size = parse_partition(PARTITIONS_CSV, SPIFFS_TYPE)
    pio_home = find_pio_home()
    assemble(args.board, args.vehicle, pio_home, fs_size,
             dry_run=args.dry_run, no_flash=args.no_flash, port=args.port)


if __name__ == "__main__":
    main()
