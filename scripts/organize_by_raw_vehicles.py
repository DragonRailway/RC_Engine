#!/usr/bin/env python3
import os
import re
import json

RAW_VEHICLES_DIR = "/home/sun/Filelink/RC_brain/sounds/raw_vehicles"
RAW_SOUNDS_DIR = "/home/sun/Filelink/RC_brain/sounds/raw_sounds"
SOUNDS_DIR = "/home/sun/Filelink/RC_brain/sounds"
VEHICLES_DIR = os.path.join(SOUNDS_DIR, "vehicles")
PRESETS_DIR = os.path.join(SOUNDS_DIR, "presets")
GENERIC_DIR = os.path.join(SOUNDS_DIR, "generic")

def parse_raw_sound_header(filepath):
    if not os.path.exists(filepath):
        return None
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    rate_match = re.search(r'SampleRate\s*=\s*(\d+)', content, re.IGNORECASE)
    sample_rate = int(rate_match.group(1)) if rate_match else 22050

    count_match = re.search(r'SampleCount\s*=\s*(\d+)', content, re.IGNORECASE)
    sample_count = int(count_match.group(1)) if count_match else 0

    array_match = re.search(r'\{([^}]+)\}', content, re.DOTALL)
    if not array_match:
        return None

    # Raw headers often carry sample-offset counters as line comments
    # (e.g. ", //16" after every 16 values). Strip comments first so the
    # offset numbers are not mistaken for audio samples.
    array_str = re.sub(r'//.*', '', array_match.group(1))

    samples = [int(num) for num in re.findall(r'-?\d+', array_str)]

    if sample_count == 0:
        sample_count = len(samples)

    return {
        "sampleRate": sample_rate,
        "sampleCount": sample_count,
        "samples": samples
    }

def clean_vehicle_name(v_filename):
    name = v_filename[:-2] # strip .h
    # Clean up name extensions
    name = re.sub(r'Excavator$', '', name)
    name = re.sub(r'Dozer$', '', name)
    name = re.sub(r'automatic$', '', name)
    name = re.sub(r'Automatic$', '', name)
    return name

def determine_slot_from_line(line, header_name):
    h_lower = header_name.lower()
    line_lower = line.lower()

    if "start" in h_lower or "start" in line_lower:
        return "start"
    elif "idle" in h_lower or "idle" in line_lower:
        return "idle"
    elif "rev" in h_lower or "rev" in line_lower:
        return "rev"
    elif "knock" in h_lower or "knock" in line_lower:
        return "knock"
    elif "jake" in h_lower or "jakebrake" in line_lower:
        return "jakebrake"
    elif "wastegate" in h_lower or "wastegate" in line_lower:
        return "wastegate"
    elif "turbo" in h_lower or "turbo" in line_lower:
        return "turbo"
    elif "charger" in h_lower or "supercharger" in h_lower:
        return "supercharger"
    elif "fan" in h_lower or "fan" in line_lower:
        return "fan"
    elif "horn" in h_lower or "horn" in line_lower or "whistle" in h_lower:
        return "horn"
    elif "siren" in h_lower or "siren" in line_lower:
        return "siren"
    elif "brake" in h_lower or "brake" in line_lower:
        return "brake"
    elif "parking" in h_lower or "parking" in line_lower:
        return "parkingbrake"
    elif "shift" in h_lower or "shift" in line_lower:
        return "shifting"
    elif "revers" in h_lower or "revers" in line_lower:
        return "reversing"
    elif "indicator" in h_lower or "indicator" in line_lower:
        return "indicator"
    elif "bell" in h_lower or "bell" in line_lower:
        return "bell"
    elif "track" in h_lower or "rattle" in h_lower:
        return "trackrattle"
    elif "bucket" in h_lower:
        return "bucketrattle"
    elif "hydraulic" in h_lower:
        return "hydraulicpump"
    elif "door" in h_lower:
        return "door"
    elif "scanner" in h_lower:
        return "scanner"
    elif "gun" in h_lower:
        return "gun"
    elif "music" in h_lower or "bond" in h_lower or "tequila" in h_lower:
        return "music"
    else:
        return "sound1"

def process_vehicle_file(v_path):
    v_filename = os.path.basename(v_path)
    veh_name = clean_vehicle_name(v_filename)
    veh_dir = os.path.join(VEHICLES_DIR, veh_name)
    os.makedirs(veh_dir, exist_ok=True)

    with open(v_path, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.readlines()

    included_count = 0

    for line in lines:
        stripped = line.strip()
        # Look for active (uncommented) #include "sounds/..." lines
        if stripped.startswith('#include') and 'sounds/' in stripped:
            match = re.search(r'sounds/([^"]+\.h)', stripped)
            if not match:
                continue
            sound_h = match.group(1)
            sound_h_path = os.path.join(RAW_SOUNDS_DIR, sound_h)

            header_base = sound_h[:-2]
            slot = determine_slot_from_line(line, header_base)

            sound_data = parse_raw_sound_header(sound_h_path)
            if not sound_data:
                continue

            target_path = os.path.join(veh_dir, f"{slot}.json")
            with open(target_path, 'w', encoding='utf-8') as f_out:
                json.dump(sound_data, f_out)

            # Also populate presets/generic if it's a shared sound
            if slot in ["brake", "shifting", "reversing", "indicator"]:
                preset_target = os.path.join(PRESETS_DIR, "heavy_truck", f"{slot}.json")
                if not os.path.exists(preset_target):
                    with open(preset_target, 'w', encoding='utf-8') as f_out:
                        json.dump(sound_data, f_out)
            elif slot in ["bell"] and "EMD" in sound_h:
                preset_target = os.path.join(PRESETS_DIR, "locomotive", f"{slot}.json")
                with open(preset_target, 'w', encoding='utf-8') as f_out:
                    json.dump(sound_data, f_out)
            elif slot in ["trackrattle", "bucketrattle", "hydraulicpump"]:
                preset_target = os.path.join(PRESETS_DIR, "excavator", f"{slot}.json")
                if not os.path.exists(preset_target):
                    with open(preset_target, 'w', encoding='utf-8') as f_out:
                        json.dump(sound_data, f_out)

            included_count += 1

    print(f"Vehicle '{veh_name}': mapped {included_count} sound slots.")

def main():
    os.makedirs(VEHICLES_DIR, exist_ok=True)
    os.makedirs(PRESETS_DIR, exist_ok=True)
    os.makedirs(GENERIC_DIR, exist_ok=True)
    os.makedirs(os.path.join(PRESETS_DIR, "heavy_truck"), exist_ok=True)
    os.makedirs(os.path.join(PRESETS_DIR, "excavator"), exist_ok=True)
    os.makedirs(os.path.join(PRESETS_DIR, "locomotive"), exist_ok=True)

    v_files = [f for f in os.listdir(RAW_VEHICLES_DIR) if f.endswith('.h') and f != '00_Master.h']
    print(f"Inspecting and organizing {len(v_files)} vehicle definitions from raw_vehicles/...")

    for v_file in v_files:
        v_path = os.path.join(RAW_VEHICLES_DIR, v_file)
        process_vehicle_file(v_path)

if __name__ == "__main__":
    main()
