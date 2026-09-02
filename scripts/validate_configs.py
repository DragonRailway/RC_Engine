#!/usr/bin/env python3
"""Validate every hardware config and vehicle bundle in the repository (CI-able).

Checks:
  1. Every config parses as JSON.
  2. Every hardware config validates against
     configs/schemas/hardware_config.schema.json.
  3. Every vehicle bundle's vehicle.json has a `vehicle` object with a
     sound_set that matches its bundle directory name (config-bundles rule),
     and its sounds/ files parse as JSON.

Exits nonzero if any check fails. Requires the `jsonschema` Python package
for the hardware schema check.

Usage:
  python3 scripts/validate_configs.py          # validate everything
"""
import json
import sys
from pathlib import Path

try:
    import jsonschema
except ImportError:
    jsonschema = None

REPO = Path(__file__).resolve().parent.parent
HW_DIR = REPO / "configs" / "hardware_configs" if (REPO / "configs" / "hardware_configs").is_dir() else (REPO / "configs")
VEHICLE_DIR = REPO / "configs" / "vehicle_configs"
SCHEMA = REPO / "configs" / "schemas" / "hardware_config.schema.json"

failures = []


def fail(msg):
    failures.append(msg)
    print(f"FAIL: {msg}")


def validate_hardware(path):
    if jsonschema is None:
        fail("jsonschema Python package required (pip install jsonschema)")
        return
    try:
        data = json.loads(path.read_text())
        schema = json.loads(SCHEMA.read_text())
    except (json.JSONDecodeError, OSError) as e:
        fail(f"{path.relative_to(REPO)}: cannot parse: {e}")
        return
    errors = sorted(jsonschema.Draft7Validator(schema).iter_errors(data),
                    key=lambda e: list(e.path) if e.path else [])
    for e in errors:
        loc = "/".join(str(p) for p in e.path) or "(root)"
        fail(f"{path.relative_to(REPO)}: {loc}: {e.message}")


def validate_bundle(bundle):
    vc = bundle / "vehicle.json"
    if not vc.is_file():
        # Sound-only bundle: valid library entry (paired with a vehicle config
        # that declares its sound_set). Only the sounds themselves are checked.
        _validate_sounds(bundle)
        return
    try:
        data = json.loads(vc.read_text())
    except (json.JSONDecodeError, OSError) as e:
        fail(f"{vc.relative_to(REPO)}: cannot parse: {e}")
        return
    veh = data.get("vehicle") or {}
    sound_set = veh.get("sound_set")
    if sound_set is None:
        fail(f"{vc.relative_to(REPO)}: missing vehicle.sound_set")
    elif sound_set != bundle.name:
        fail(f"{vc.relative_to(REPO)}: sound_set '{sound_set}' != bundle dir '{bundle.name}'")
    _validate_sounds(bundle)


def _validate_sounds(bundle):
    sounds = bundle / "sounds"
    if sounds.is_dir():
        for slot in sorted(sounds.glob("*.json")):
            try:
                json.loads(slot.read_text())
            except (json.JSONDecodeError, OSError) as e:
                fail(f"{slot.relative_to(REPO)}: cannot parse: {e}")


def main():
    if not SCHEMA.is_file():
        fail(f"schema not found: {SCHEMA.relative_to(REPO)}")

    for hw in sorted(HW_DIR.glob("hardware-*.json")):
        validate_hardware(hw)

    for bundle in sorted(p for p in VEHICLE_DIR.iterdir() if p.is_dir() and p.name != "common"):
        validate_bundle(bundle)

    # Common preset fallbacks must also parse as JSON.
    common = VEHICLE_DIR / "common"
    if common.is_dir():
        for slot in sorted(common.rglob("*.json")):
            try:
                json.loads(slot.read_text())
            except (json.JSONDecodeError, OSError) as e:
                fail(f"{slot.relative_to(REPO)}: cannot parse: {e}")

    if failures:
        print(f"\n{len(failures)} validation failure(s)")
        sys.exit(1)
    print(f"OK: {len(list(HW_DIR.glob('hardware-*.json')))} hardware configs, "
          f"{len([p for p in VEHICLE_DIR.iterdir() if p.is_dir() and p.name != 'common'])} bundles "
          "validated")


if __name__ == "__main__":
    main()
