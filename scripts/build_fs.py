#!/usr/bin/env python3
"""Manifest-driven LittleFS deployment for RC_Engine (config-bundles model).

Assembles a board's LittleFS image from the configs/ library — exactly one
hardware config + one vehicle bundle (+ shared preset fallbacks) — and flashes
it to the spiffs partition. This is the ONLY supported FS-flash path; the
PlatformIO `uploadfs` target builds from the hardcoded `data/` tree and always
fails with LFS_ERR_NOSPC (the full repo tree is ~132 MB vs an 896 KB partition).

Layout mapping (repo -> board LittleFS):
  configs/hardware_configs/hardware-<BOARD>.json -> /hardware-<BOARD>.json
  configs/vehicle_configs/<V>/vehicle.json       -> /vehicle-config.json
  configs/vehicle_configs/<V>/sounds/<slot>.pcm  -> /sounds/vehicles/<V>/<slot>.pcm
  configs/vehicle_configs/common/<preset>/...    -> /sounds/presets/<preset>/...
      (only for slots the vehicle bundle does NOT provide — tier-2 fallback)

The image is built with the same `littlefs` Python package (and identical
parameters) that the pioarduino espressif32 platform uses for `uploadfs`, so
the output is byte-for-byte compatible with what the firmware expects.

Usage:
  scripts/build_fs.py --board MIKRO_V2 --vehicle ScaniaV8            # build + flash
  scripts/build_fs.py --board MIKRO_V2 --vehicle ScaniaV8 --dry-run  # size report only
  scripts/build_fs.py --board MIKRO_V2 --vehicle ScaniaV8 --no-flash # build image only
  scripts/build_fs.py --board MIKRO_V2 --vehicle ScaniaV8 --hardware skid  # skid-steer hw config
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

try:
    import jsonschema
except ImportError:
    jsonschema = None

REPO = Path(__file__).resolve().parent.parent
PARTITIONS_CSV = REPO / "partitions_ota_4MB.csv"
CONFIGS = REPO / "configs"
HW_DIR = CONFIGS / "hardware_configs"
VEHICLE_DIR = CONFIGS / "vehicle_configs"
COMMON_DIR = VEHICLE_DIR / "common"
SCHEMAS_DIR = CONFIGS / "schemas"
STAGING_ROOT = REPO / ".pio" / "fs_staging"


def validate_hardware_config(hw_path):
    """Validate a hardware config against configs/schemas/hardware_config.schema.json.

    Fails fast with the violations listed. Requires the `jsonschema` Python
    package; if it is missing, die with a clear message (a hard stop is safer
    than silently skipping the check).
    """
    schema_path = SCHEMAS_DIR / "hardware_config.schema.json"
    if not schema_path.is_file():
        die(f"hardware config schema not found: {schema_path.relative_to(REPO)}")
    if jsonschema is None:
        die("jsonschema Python package is required for config validation "
            "(pip install jsonschema)")

    try:
        with open(hw_path) as f:
            data = json.load(f)
        with open(schema_path) as f:
            schema = json.load(f)
    except (json.JSONDecodeError, OSError) as e:
        die(f"cannot parse {hw_path.relative_to(REPO)} or schema: {e}")

    validator = jsonschema.Draft7Validator(schema)
    errors = sorted(validator.iter_errors(data),
                    key=lambda e: list(e.path) if e.path else [])
    if errors:
        lines = [f"hardware config {hw_path.relative_to(REPO)} failed validation:"]
        for e in errors:
            loc = "/".join(str(p) for p in e.path) or "(root)"
            lines.append(f"  {loc}: {e.message}")
        die("\n".join(lines))

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
        base_board = board.split("-")[0]
        section = f"env:{base_board}"
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


def assemble(board, vehicle, pio_home, fs_size, dry_run=False, no_flash=False,
             port=None, hardware=None):
    # The repo names hardware configs descriptively (hardware-MIKRO_V2-truck.json,
    # hardware-TRACKLINK_V3-locomotive.json), while the firmware always loads
    # /hardware-<BOARD>.json. An explicit variant (--hardware skid →
    # hardware-MIKRO_V2-skid.json) wins; otherwise accept either the bare name
    # or a unique hardware-<BOARD>-*.json match, and stage it under the
    # firmware's name.
    hw_src = HW_DIR / f"hardware-{board}.json"
    if hardware:
        hw_src = HW_DIR / f"hardware-{board}-{hardware}.json"
        if not hw_src.is_file():
            die(f"hardware config variant not found: {hw_src.relative_to(HW_DIR)}")
    elif not hw_src.is_file():
        matches = sorted(HW_DIR.glob(f"hardware-{board}-*.json"))
        if len(matches) == 1:
            hw_src = matches[0]
        elif len(matches) > 1:
            die(f"multiple hardware configs match board '{board}': "
                + ", ".join(m.relative_to(HW_DIR).name for m in matches))
    if not hw_src.is_file():
        die(f"hardware config not found: {HW_DIR / f'hardware-{board}.json'} "
            f"(looked for hardware-{board}.json / hardware-{board}-*.json)")
    validate_hardware_config(hw_src)
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
        veh = vc.get("vehicle") or {}
        vtype = (veh.get("type") or "truck").lower()
    except (json.JSONDecodeError, OSError) as e:
        die(f"cannot parse {vc_src.relative_to(REPO)}: {e}")

    staging = STAGING_ROOT / f"{board}-{vehicle}"
    if staging.exists():
        shutil.rmtree(staging)
    staging.mkdir(parents=True)

    # /hardware-<BOARD>.json (always the bare board name — the firmware's
    # HW_CONFIG_PATH is /hardware-<BOARD>.json regardless of the repo suffix).
    shutil.copy2(hw_src, staging / f"hardware-{board}.json")
    # /vehicle-config.json
    shutil.copy2(vc_src, staging / "vehicle-config.json")
    # /sounds/vehicles/<V>/<slot>.pcm
    (staging / "sounds" / "vehicles" / vehicle).mkdir(parents=True)
    for slot in sorted(sounds_src.glob("*.pcm")):
        shutil.copy2(slot, staging / "sounds" / "vehicles" / vehicle / slot.name)

    # Tier-2 type-based common fallbacks: only slots the vehicle bundle does NOT provide.
    vehicle_slots = {s.name for s in sounds_src.glob("*.pcm")}
    common_type_dir = COMMON_DIR / vtype
    fallbacks = [s for s in sorted(common_type_dir.glob("*.pcm")) if s.name not in vehicle_slots] if common_type_dir.is_dir() else []
    if fallbacks:
        (staging / "sounds" / "common" / vtype).mkdir(parents=True)
        for slot in fallbacks:
            shutil.copy2(slot, staging / "sounds" / "common" / vtype / slot.name)

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
    ap.add_argument("--hardware", default=None,
                    help="hardware config variant, e.g. skid (selects hardware-<BOARD>-<variant>.json)")
    args = ap.parse_args()

    if not PARTITIONS_CSV.is_file():
        die(f"partitions file not found: {PARTITIONS_CSV}")
    offset, fs_size = parse_partition(PARTITIONS_CSV, SPIFFS_TYPE)
    pio_home = find_pio_home()
    assemble(args.board, args.vehicle, pio_home, fs_size,
             dry_run=args.dry_run, no_flash=args.no_flash, port=args.port,
             hardware=args.hardware)


if __name__ == "__main__":
    main()
