#!/usr/bin/env python3
import os
import re
import json
import subprocess

RAW_SOUNDS_DIR = os.path.join(os.path.dirname(__file__), "..", "references", "raw_sounds")
# Loose <slot>-<Vehicle>.json output lands in gitignored scratch (data/);
# organize_sounds.py moves them into the configs/vehicle_configs bundles.
SOUNDS_DIR = os.path.join(os.path.dirname(__file__), "..", "data")

def parse_header_sound(filepath):
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    # Find sampleRate
    rate_match = re.search(r'SampleRate\s*=\s*(\d+)', content, re.IGNORECASE)
    sample_rate = int(rate_match.group(1)) if rate_match else 22050

    # Find sampleCount
    count_match = re.search(r'SampleCount\s*=\s*(\d+)', content, re.IGNORECASE)
    sample_count = int(count_match.group(1)) if count_match else 0

    # Find samples array between { ... }
    array_match = re.search(r'\{([^}]+)\}', content, re.DOTALL)
    if not array_match:
        return None

    array_str = array_match.group(1)
    # Raw headers often carry sample-offset counters as line comments
    # (e.g. ", //16" after every 16 values). Strip comments first so the
    # offset numbers are not mistaken for audio samples.
    array_str = re.sub(r'//.*', '', array_str)
    # Extract all numbers (positive and negative integers)
    samples = [int(num) for num in re.findall(r'-?\d+', array_str)]

    if sample_count == 0:
        sample_count = len(samples)

    return {
        "sampleRate": sample_rate,
        "sampleCount": sample_count,
        "samples": samples
    }

def camel_to_snake(name):
    # Map common header name patterns to standard filename format <slot>-<Vehicle>
    pass

def map_header_filename_to_json_filename(h_filename):
    base = h_filename[:-2] # remove .h
    
    # Common mappings
    # e.g., ScaniaV8idle.h -> idle-ScaniaV8.json
    # e.g., Caterpillar323Idle.h -> idle-Caterpillar323.json
    # e.g., EMDLocomotiveBell.h -> bell-EMDLocomotive.json
    # e.g., HornblastersOUTLAWTrainHornLong.h -> horn-blastersOUTLAWTrainHornLong.json
    
    # Check prefixes/suffixes for sound types
    types = [
        ("Idle", "idle"), ("idle", "idle"),
        ("Rev", "rev"), ("rev", "rev"),
        ("Start", "start"), ("start", "start"),
        ("Knock", "knock"), ("knock", "knock"),
        ("Horn", "horn"), ("horn", "horn"),
        ("Turbo", "turbo"), ("turbo", "turbo"),
        ("Wastegate", "wastegate"), ("wastegate", "wastegate"),
        ("JakeBrake", "jakebrake"), ("jakeBrake", "jakebrake"), ("jakebrake", "jakebrake"),
        ("AirBrakes", "brake"), ("AirBrake", "brake"), ("brake", "brake"),
        ("ParkingBrake", "parkingbrake"), ("parkingBrake", "parkingbrake"),
        ("AirShifting", "shifting"), ("Shifting", "shifting"), ("shifting", "shifting"),
        ("ReversingBeep", "reversing"), ("reversing", "reversing"),
        ("Indicator", "indicator"), ("indicator", "indicator"),
        ("Supercharger", "supercharger"), ("supercharger", "supercharger"),
        ("TrackRattle", "trackrattle"), ("trackrattle", "trackrattle"),
        ("Bucket", "bucketrattle"),
        ("Bell", "bell"), ("bell", "bell"),
        ("Door", "door"), ("door", "door"),
        ("Scanner", "scanner"), ("scanner", "scanner"),
        ("Whistle", "whistle"), ("whistle", "whistle"),
        ("Guns", "gun"), ("gun", "gun"),
        ("Fan", "fan"), ("fan", "fan"),
        ("Roar", "roar"), ("roar", "roar"),
        ("Squeal", "tiresqueal"), ("squeal", "tiresqueal")
    ]

    found_type = None
    veh_name = base

    # Try matching type in filename
    for t_str, t_clean in types:
        if base.endswith(t_str):
            found_type = t_clean
            veh_name = base[:-len(t_str)]
            break
        elif base.startswith(t_str):
            found_type = t_clean
            veh_name = base[len(t_str):]
            break

    if found_type and veh_name:
        return f"{found_type}-{veh_name}.json"
    else:
        return f"{base}.json"

def main():
    if not os.path.exists(RAW_SOUNDS_DIR):
        print("No raw_sounds directory found.")
        return

    header_files = [f for f in os.listdir(RAW_SOUNDS_DIR) if f.endswith('.h')]
    print(f"Found {len(header_files)} raw header sound files.")

    converted_count = 0
    for h_file in header_files:
        h_path = os.path.join(RAW_SOUNDS_DIR, h_file)
        data = parse_header_sound(h_path)
        if not data:
            print(f"Skipping {h_file} (could not parse)")
            continue

        json_name = map_header_filename_to_json_filename(h_file)
        out_path = os.path.join(SOUNDS_DIR, json_name)

        with open(out_path, 'w', encoding='utf-8') as f:
            json.dump(data, f)
        converted_count += 1

    print(f"Successfully converted {converted_count} C header sound files into JSON format.")

if __name__ == "__main__":
    main()
