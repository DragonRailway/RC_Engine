#!/usr/bin/env python3
"""Sound Data Validator for RC_Engine (config-bundles layout).

Walks configs/vehicle_configs/ to validate:
- Bundle layout: bundle dir name == the `sound_set` in its vehicle.json;
  every vehicle has a vehicle.json manifest and a sounds/ subdirectory
- Sound JSON validity: sampleRate is 22,050 Hz; sampleCount > 0 and matches
  the samples array length; loop points satisfy 0 <= begin < end <= count
- Signal stats per sound: peak, RMS, DC offset, silence, clipping

Exits nonzero if any file or bundle fails validation checks.
"""
import os
import sys
import json
import math

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
VEHICLE_DIR = os.path.join(REPO, "configs", "vehicle_configs")


def validate_sound_file(filepath):
    errors = []
    warnings = []

    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            data = json.load(f)
    except Exception as e:
        return False, [f"JSON parse error: {e}"], [], {}

    # If it's a vehicle config, validate loop_points section if present
    if "loop_points" in data:
        lp = data["loop_points"]
        for prefix in ["horn", "siren", "reversing", "sound1"]:
            b_key = f"{prefix}_begin"
            e_key = f"{prefix}_end"
            begin = lp.get(b_key, 0)
            end = lp.get(e_key, 0)
            if end > 0:
                if begin < 0 or begin >= end:
                    errors.append(f"Invalid loop points for {prefix}: begin={begin}, end={end} (must satisfy 0 <= begin < end)")
        return len(errors) == 0, errors, warnings, {}

    # Standard sound JSON file validation
    if "samples" not in data:
        # Not a raw sound data file (e.g. metadata only)
        return True, [], ["No 'samples' key found"], {}

    sample_rate = data.get("sampleRate")
    if sample_rate is None:
        errors.append("Missing 'sampleRate' field")
    elif sample_rate != 22050:
        errors.append(f"Invalid sampleRate: {sample_rate} Hz (expected 22050 Hz)")

    samples = data.get("samples", [])
    sample_count = data.get("sampleCount", len(samples))

    if sample_count <= 0:
        errors.append(f"Invalid sampleCount: {sample_count} (must be > 0)")

    if len(samples) != sample_count:
        errors.append(f"sampleCount mismatch: declared {sample_count}, found {len(samples)} samples")

    # Check for direct loop point fields if specified in sound file
    for b_key, e_key in [("loopBegin", "loopEnd"), ("hornBegin", "hornEnd"), ("begin", "end")]:
        if b_key in data or e_key in data:
            begin = data.get(b_key, 0)
            end = data.get(e_key, 0)
            if end > 0:
                if begin < 0 or begin >= end or end > len(samples):
                    errors.append(f"Invalid loop points ({b_key}/{e_key}): begin={begin}, end={end}, count={len(samples)}")

    if not samples:
        errors.append("Empty 'samples' array")
        return False, errors, warnings, {}

    # Signal stats calculation
    min_val = min(samples)
    max_val = max(samples)
    peak = max(abs(min_val), abs(max_val))
    dc_offset = sum(samples) / len(samples)
    sum_sq = sum(s * s for s in samples)
    rms = math.sqrt(sum_sq / len(samples))
    clipping_count = sum(1 for s in samples if s <= -128 or s >= 127)

    if peak == 0:
        warnings.append("All-zero / silent sound file")

    if clipping_count > 0:
        warnings.append(f"{clipping_count} samples clipped at int8 bounds (-128 / 127)")

    if abs(dc_offset) > 10.0:
        warnings.append(f"High DC offset detected: {dc_offset:.2f}")

    stats = {
        "count": len(samples),
        "peak": peak,
        "rms": round(rms, 2),
        "dc": round(dc_offset, 2),
        "clips": clipping_count,
    }
    return len(errors) == 0, errors, warnings, stats


def validate_bundle(bundle_dir, bundle_name):
    """Check layout: sound_set == dir name (when a manifest exists), sounds/ subdir.

    Sound-only bundles (sounds/ with no vehicle.json) are valid library entries:
    they stage as /sounds/vehicles/<set>/ and can be paired with any vehicle
    config that declares their sound_set. The dangerous case we guard against is
    a bundle whose dir name does NOT match the sound_set it declares.
    """
    errors = []
    vc_path = os.path.join(bundle_dir, "vehicle.json")
    if os.path.isfile(vc_path):
        try:
            with open(vc_path) as f:
                vc = json.load(f)
        except Exception as e:
            return False, [f"bundle '{bundle_name}' vehicle.json unparsable: {e}"]
        sound_set = (vc.get("vehicle") or {}).get("sound_set")
        if sound_set != bundle_name:
            errors.append(
                f"bundle dir '{bundle_name}' != sound_set '{sound_set}' in vehicle.json "
                "(dir name must match sound_set or firmware sound resolution breaks)")

    sounds_dir = os.path.join(bundle_dir, "sounds")
    if not os.path.isdir(sounds_dir):
        errors.append(f"bundle '{bundle_name}' has no sounds/ subdirectory")
    return len(errors) == 0, errors


def main():
    if not os.path.isdir(VEHICLE_DIR):
        sys.stderr.write(f"ERROR: vehicle configs dir not found: {VEHICLE_DIR}\n")
        sys.exit(1)

    print(f"Scanning bundles in: {VEHICLE_DIR}")
    total_files = 0
    passed_files = 0
    failed_files = 0
    bundle_failures = 0

    for bundle_name in sorted(os.listdir(VEHICLE_DIR)):
        bundle_dir = os.path.join(VEHICLE_DIR, bundle_name)
        if not os.path.isdir(bundle_dir):
            continue
        # common/ holds shared preset fallbacks (per-preset subdirs), not bundles
        if bundle_name == "common":
            continue

        # Validate bundle layout (dir name == sound_set, manifest + sounds/ present)
        ok, bundle_errors = validate_bundle(bundle_dir, bundle_name)
        if not ok:
            bundle_failures += 1
            for err in bundle_errors:
                print(f"  [BUNDLE FAIL] {bundle_name}: {err}")

        # Validate each sound file under sounds/
        sounds_dir = os.path.join(bundle_dir, "sounds")
        if not os.path.isdir(sounds_dir):
            continue
        for f in sorted(os.listdir(sounds_dir)):
            if not f.endswith(".json"):
                continue
            total_files += 1
            filepath = os.path.join(sounds_dir, f)
            relpath = os.path.join(bundle_name, "sounds", f)

            res = validate_sound_file(filepath)
            ok, errors, warnings, stats = res[0], res[1], res[2], (res[3] if len(res) > 3 else {})

            if ok:
                passed_files += 1
                stat_str = (f" (N={stats.get('count', 0)}, peak={stats.get('peak', 0)}, "
                            f"RMS={stats.get('rms', 0)}, DC={stats.get('dc', 0)})") if stats else ""
                warn_str = f" [WARN: {', '.join(warnings)}]" if warnings else ""
                print(f"  [PASS] {relpath}{stat_str}{warn_str}")
            else:
                failed_files += 1
                print(f"  [FAIL] {relpath}")
                for err in errors:
                    print(f"    - ERROR: {err}")

    print(f"\nValidation Summary: {passed_files}/{total_files} sound files passed, "
          f"{failed_files} failed, {bundle_failures} bundle layout failures.")
    if failed_files > 0 or bundle_failures > 0:
        sys.exit(1)
    sys.exit(0)


if __name__ == "__main__":
    main()
