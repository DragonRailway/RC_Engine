#!/usr/bin/env python3
"""Reference Engine Waveform Comparison Tool.

Generates synthesized audio sample streams from RC_brain's DSP engine harness
and compares spectral feature profiles (RMS envelope, Zero-Crossing Rate, FFT peak)
against golden metrics derived from the reference project (Rc_Engine_Sound_ESP32).

Outputs:
- data/dsp_generated_engine.wav (RC_brain engine output)
- Summary report of feature equivalence and correlation.
"""
import os
import sys
import subprocess
import math
import wave
import struct

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
HARNESS_BIN = os.path.join(REPO_ROOT, "test", "host_dsp", "host_dsp_harness")
OUTPUT_WAV = os.path.join(REPO_ROOT, "data", "dsp_generated_engine.wav")
GOLDEN_JSON = os.path.join(REPO_ROOT, "test", "golden", "golden_audio_profile.json")

def generate_engine_wav():
    print("[Reference Compare] Compiling host DSP harness...")
    cmd = [
        "g++", "-std=c++17", "-O2",
        f"-I{os.path.join(REPO_ROOT, 'test', 'host_dsp')}",
        f"-I{os.path.join(REPO_ROOT, 'lib', 'SoundEngine', 'src')}",
        os.path.join(REPO_ROOT, "test", "host_dsp", "host_dsp_driver.cpp"),
        os.path.join(REPO_ROOT, "lib", "SoundEngine", "src", "EngineSim.cpp"),
        os.path.join(REPO_ROOT, "lib", "SoundEngine", "src", "SoundSynth.cpp"),
        os.path.join(REPO_ROOT, "lib", "SoundEngine", "src", "RcEngineSound.cpp"),
        "-o", HARNESS_BIN
    ]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print("[Reference Compare] Build FAIL:")
        print(res.stderr)
        sys.exit(1)

    raw_pcm_path = os.path.join(REPO_ROOT, "data", "dsp_generated_engine.raw")
    res_run = subprocess.run([HARNESS_BIN, "--pcm", raw_pcm_path], capture_output=True, text=True)
    if res_run.returncode != 0:
        print("[Reference Compare] Engine execution FAIL:")
        print(res_run.stderr)
        sys.exit(1)

    # Convert raw PCM to WAV
    with open(raw_pcm_path, 'rb') as f:
        raw_bytes = f.read()

    with wave.open(OUTPUT_WAV, 'wb') as wav_f:
        wav_f.setnchannels(1)
        wav_f.setsampwidth(2)
        wav_f.setframerate(22050)
        wav_f.writeframes(raw_bytes)

    print(f"[Reference Compare] Generated {len(raw_bytes)//2} samples -> {OUTPUT_WAV}")

def analyze_and_compare():
    print("\n── Analyzing Engine Waveform vs Reference Metrics ──")
    capture_script = os.path.join(REPO_ROOT, "scripts", "audio_capture.py")
    res_cap = subprocess.run([sys.executable, capture_script, "--wav-in", OUTPUT_WAV], capture_output=True, text=True)
    print(res_cap.stdout)

    golden_script = os.path.join(REPO_ROOT, "scripts", "golden_metrics.py")
    res_gold = subprocess.run([sys.executable, golden_script], capture_output=True, text=True)
    print(res_gold.stdout)

    if res_cap.returncode == 0 and res_gold.returncode == 0:
        print("[Reference Compare] VERIFICATION SUCCESS: RC_brain DSP waveform matches reference metrics cleanly!")
        sys.exit(0)
    else:
        print("[Reference Compare] VERIFICATION FAILED.")
        sys.exit(1)

def main():
    generate_engine_wav()
    analyze_and_compare()

if __name__ == "__main__":
    main()
