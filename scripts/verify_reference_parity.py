#!/usr/bin/env python3
"""Comprehensive Multi-Scenario Reference Parity Suite.

Evaluates 5 control scenarios against the reference project (Rc_Engine_Sound_ESP32):
  1. Slow Throttle Ramp (0% -> 30% -> 70% -> 100% -> 0%)
  2. Rapid Throttle Pulse & Deceleration
  3. Reversing Beep & Gear Shift Sequence
  4. Turn Indicator & Horn Overlay
  5. Full Stress Mixed Combination (Throttle + Horn + Indicator + Reversing)

Outputs exact Pearson correlation (r), ZCR frequency match, MAE, and pass status.
"""
import os
import sys
import subprocess
import math
import wave
import struct

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
COMPARE_HARNESS_SRC = os.path.join(REPO_ROOT, "test", "host_dsp", "compare_harness.cpp")
COMPARE_BIN = os.path.join(REPO_ROOT, "test", "host_dsp", "compare_harness")

SCENARIOS = [
    (1, "Slow Throttle Ramp (0 -> 30 -> 70 -> 100%)"),
    (2, "Rapid Throttle Pulse & Deceleration"),
    (3, "Reversing Beep & Reverse Gear"),
    (4, "Turn Signal Indicator & Horn Overlay"),
    (5, "Full Stress Mixed Combination Load")
]

def compile_compare_harness():
    cmd = [
        "g++", "-std=c++17", "-O2",
        f"-I{os.path.join(REPO_ROOT, 'test', 'host_dsp')}",
        f"-I{os.path.join(REPO_ROOT, 'lib', 'SoundEngine', 'src')}",
        COMPARE_HARNESS_SRC,
        os.path.join(REPO_ROOT, "lib", "SoundEngine", "src", "RcEngineSound.cpp"),
        "-o", COMPARE_BIN
    ]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print("[Parity Suite] Compilation FAIL:")
        print(res.stderr)
        sys.exit(1)

def run_scenario(scenario_id):
    ref_raw = os.path.join(REPO_ROOT, "data", f"ref_scen_{scenario_id}.raw")
    rcb_raw = os.path.join(REPO_ROOT, "data", f"rcb_scen_{scenario_id}.raw")
    ref_wav = os.path.join(REPO_ROOT, "data", f"ref_scen_{scenario_id}.wav")
    rcb_wav = os.path.join(REPO_ROOT, "data", f"rcb_scen_{scenario_id}.wav")

    res = subprocess.run([
        COMPARE_BIN,
        "--scenario", str(scenario_id),
        "--ref-out", ref_raw,
        "--rcb-out", rcb_raw
    ], capture_output=True, text=True)

    if res.returncode != 0:
        print(f"[Parity Suite] Scenario {scenario_id} FAIL:")
        print(res.stderr)
        sys.exit(1)

    for raw_path, wav_path in [(ref_raw, ref_wav), (rcb_raw, rcb_wav)]:
        with open(raw_path, 'rb') as f:
            raw_bytes = f.read()
        with wave.open(wav_path, 'wb') as w:
            w.setnchannels(1)
            w.setsampwidth(2)
            w.setframerate(22050)
            w.writeframes(raw_bytes)

    # Read WAV samples for comparison
    with wave.open(ref_wav, 'rb') as w_ref, wave.open(rcb_wav, 'rb') as w_rcb:
        ref_shorts = list(struct.unpack(f'<{w_ref.getnframes()}h', w_ref.readframes(w_ref.getnframes())))
        rcb_shorts = list(struct.unpack(f'<{w_rcb.getnframes()}h', w_rcb.readframes(w_rcb.getnframes())))

    N = min(len(ref_shorts), len(rcb_shorts))
    mean_ref = sum(ref_shorts[:N]) / float(N)
    mean_rcb = sum(rcb_shorts[:N]) / float(N)

    num = 0.0
    den_ref = 0.0
    den_rcb = 0.0
    total_abs_err = 0.0

    for i in range(N):
        diff_ref = ref_shorts[i] - mean_ref
        diff_rcb = rcb_shorts[i] - mean_rcb
        num += diff_ref * diff_rcb
        den_ref += diff_ref * diff_ref
        den_rcb += diff_rcb * diff_rcb
        total_abs_err += abs(ref_shorts[i] - rcb_shorts[i])

    corr = num / math.sqrt(den_ref * den_rcb) if (den_ref > 0 and den_rcb > 0) else 0.0
    mae = total_abs_err / float(N)

    zcr_ref = sum(1 for i in range(1, N) if (ref_shorts[i-1] >= 0 and ref_shorts[i] < 0) or (ref_shorts[i-1] < 0 and ref_shorts[i] >= 0))
    zcr_rcb = sum(1 for i in range(1, N) if (rcb_shorts[i-1] >= 0 and rcb_shorts[i] < 0) or (rcb_shorts[i-1] < 0 and rcb_shorts[i] >= 0))

    zcr_freq_ref = (zcr_ref / float(N)) * 22050.0
    zcr_freq_rcb = (zcr_rcb / float(N)) * 22050.0

    return corr, mae, zcr_freq_ref, zcr_freq_rcb

def main():
    compile_compare_harness()

    print("\n==========================================================================================")
    print("── COMPREHENSIVE MULTI-SCENARIO REFERENCE PARITY MATRIX ──")
    print("==========================================================================================")
    print(f"{'ID':<3} | {'Scenario Name':<42} | {'Correlation (r)':<15} | {'ZCR Ref / RCB':<16} | {'Status':<6}")
    print("------------------------------------------------------------------------------------------")

    all_passed = True
    for s_id, s_name in SCENARIOS:
        corr, mae, zcr_ref, zcr_rcb = run_scenario(s_id)
        status = "PASS" if corr >= 0.94 else "FAIL"
        if status == "FAIL":
            all_passed = False
        print(f"{s_id:<3} | {s_name:<42} | {corr*100:.2f}% (r={corr:.4f}) | {zcr_ref:.1f}Hz / {zcr_rcb:.1f}Hz | {status:<6}")

    print("==========================================================================================")
    if all_passed:
        print("[PARITY SUITE SUCCESS] All 5 multi-combination scenarios matched reference project with >94% correlation!")
        sys.exit(0)
    else:
        print("[PARITY SUITE FAIL] One or more scenarios failed parity thresholds.")
        sys.exit(1)

if __name__ == "__main__":
    main()
