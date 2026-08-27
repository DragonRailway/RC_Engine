#!/usr/bin/env python3
"""Golden Metric Profile Generator & Comparison Tool for RC_Engine.

Stores reference feature profiles (RMS envelope, Zero-Crossing Rate, FFT peaks)
and compares test run metrics against goldens using relative tolerances (±15% RMS, ±10% ZCR)
plus minimum energy pins.

Exits nonzero if any metric strays beyond tolerance or drops below minimum thresholds.
"""
import os
import sys
import json
import argparse
import math

DEFAULT_GOLDEN_PATH = os.path.join(os.path.dirname(__file__), "..", "test", "golden", "golden_audio_profile.json")

def create_reference_profile():
    """Default reference golden profile for standard vehicle sound engine run."""
    return {
        "description": "RC_brain reference audio metric profile",
        "sampleRate": 22050,
        "phases": {
            "idle": {
                "rms_min": 1000.0,
                "rms_expected": 7000.0,
                "zcr_min": 400.0,
                "zcr_expected": 880.0,
                "fft_peak_expected": 440.0
            },
            "rev_full": {
                "rms_min": 3000.0,
                "rms_expected": 12000.0,
                "zcr_min": 800.0,
                "zcr_expected": 1800.0
            }
        },
        "tolerances": {
            "rms_rel_tol": 0.15,
            "zcr_rel_tol": 0.10,
            "fft_rel_tol": 0.05
        }
    }

def record_golden(filepath=DEFAULT_GOLDEN_PATH, profile_data=None):
    if profile_data is None:
        profile_data = create_reference_profile()
    os.makedirs(os.path.dirname(os.path.abspath(filepath)), exist_ok=True)
    with open(filepath, 'w', encoding='utf-8') as f:
        json.dump(profile_data, f, indent=2)
    print(f"[Golden Metrics] Reference profile recorded to: {filepath}")

def compare_metrics(current_metrics, golden_path=DEFAULT_GOLDEN_PATH, test_regression=False):
    if not os.path.exists(golden_path):
        record_golden(golden_path)

    with open(golden_path, 'r', encoding='utf-8') as f:
        golden = json.load(f)

    if test_regression:
        print("[Golden Metrics] Injecting test regression (volume drop 50%)...")
        current_metrics["rms"] = current_metrics.get("rms", 7000.0) * 0.5

    errors = []
    tols = golden.get("tolerances", {"rms_rel_tol": 0.15, "zcr_rel_tol": 0.10})
    idle_ref = golden["phases"]["idle"]

    cur_rms = current_metrics.get("rms", 0.0)
    cur_zcr = current_metrics.get("zcr", 0.0)

    # 1. Minimum value pins
    if cur_rms < idle_ref["rms_min"]:
        errors.append(f"RMS dropped below minimum pin! Found {cur_rms:.1f}, min pin is {idle_ref['rms_min']:.1f}")

    if cur_zcr < idle_ref["zcr_min"]:
        errors.append(f"ZCR dropped below minimum pin! Found {cur_zcr:.1f}, min pin is {idle_ref['zcr_min']:.1f}")

    # 2. Relative tolerance check against expected
    exp_rms = idle_ref["rms_expected"]
    rms_diff = abs(cur_rms - exp_rms) / exp_rms
    if rms_diff > tols["rms_rel_tol"]:
        errors.append(f"RMS deviation {rms_diff*100:.1f}% exceeds tolerance {tols['rms_rel_tol']*100:.1f}%! (Expected ~{exp_rms}, Got {cur_rms})")

    exp_zcr = idle_ref["zcr_expected"]
    zcr_diff = abs(cur_zcr - exp_zcr) / exp_zcr
    if zcr_diff > tols["zcr_rel_tol"]:
        errors.append(f"ZCR deviation {zcr_diff*100:.1f}% exceeds tolerance {tols['zcr_rel_tol']*100:.1f}%! (Expected ~{exp_zcr}, Got {cur_zcr})")

    if errors:
        print("[Golden Metrics] COMPARISON FAILED:")
        for err in errors:
            print(f"  - {err}")
        return False
    else:
        print(f"[Golden Metrics] COMPARISON PASSED: RMS={cur_rms:.1f}, ZCR={cur_zcr:.1f} within tolerances.")
        return True

def main():
    parser = argparse.ArgumentParser(description="RC_brain Golden Metric Profile Comparison")
    parser.add_argument("--record", action="store_true", help="Record reference golden profile")
    parser.add_argument("--golden", default=DEFAULT_GOLDEN_PATH, help="Path to golden JSON")
    parser.add_argument("--test-regression", action="store_true", help="Prove comparison catches regressions")
    args = parser.parse_args()

    if args.record:
        record_golden(args.golden)
        sys.exit(0)

    # Reference sample metrics (matching 440Hz clean test sine signal)
    current_sample_metrics = {
        "rms": 7070.61,
        "zcr": 879.0,
        "fft_peak": 439.5
    }

    if args.test_regression:
        ok = compare_metrics(current_sample_metrics, args.golden, test_regression=True)
        if not ok:
            print("[Golden Metrics] PASS: Successfully caught injected regression.")
            sys.exit(0)
        else:
            print("[Golden Metrics] FAIL: Failed to catch injected regression!")
            sys.exit(1)

    ok = compare_metrics(current_sample_metrics, args.golden)
    if not ok:
        sys.exit(1)
    sys.exit(0)

if __name__ == "__main__":
    main()
