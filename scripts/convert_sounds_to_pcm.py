#!/usr/bin/env python3
"""Batch converter: Converts all vehicle sound JSON files to 8-byte header .pcm binary files."""
import os
import sys
import json
import struct

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
VEHICLE_DIR = os.path.join(REPO, "configs", "vehicle_configs")

def convert_sound_file(json_path):
    with open(json_path, 'r', encoding='utf-8') as f:
        try:
            data = json.load(f)
        except Exception as e:
            print(f"Error parsing {json_path}: {e}")
            return False

    if not isinstance(data, dict) or "samples" not in data:
        # Not a sound file (e.g. vehicle manifest)
        return False

    sample_rate = data.get("sampleRate", 22050)
    samples = data.get("samples", [])
    sample_count = len(samples)

    if sample_count == 0:
        print(f"Warning: Empty samples in {json_path}")
        return False

    # Convert samples to signed 8-bit integers (-128 to 127)
    byte_array = bytearray(sample_count)
    for i, s in enumerate(samples):
        v = int(s)
        if v < -128: v = -128
        elif v > 127: v = 127
        byte_array[i] = v & 0xFF

    pcm_path = os.path.splitext(json_path)[0] + ".pcm"
    header = struct.pack('<2sHI', b'RP', sample_rate, sample_count)

    with open(pcm_path, 'wb') as f:
        f.write(header)
        f.write(byte_array)

    os.remove(json_path)
    return True

def main():
    total_converted = 0
    total_failed = 0

    print(f"Converting JSON sound files in {VEHICLE_DIR} to .pcm...")
    for root, dirs, files in os.walk(VEHICLE_DIR):
        for file in files:
            if file.endswith(".json") and file != "vehicle.json":
                full_path = os.path.join(root, file)
                if convert_sound_file(full_path):
                    total_converted += 1
                else:
                    total_failed += 1

    print(f"Conversion complete: {total_converted} files converted to .pcm, {total_failed} non-sound files skipped.")

if __name__ == "__main__":
    main()
