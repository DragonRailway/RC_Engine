#!/usr/bin/env python3
import os
import shutil

SCRATCH_DIR = os.path.join(os.path.dirname(__file__), "..", "data")  # loose JSON in/out (gitignored)
VEHICLES_DIR = os.path.join(os.path.dirname(__file__), "..", "configs", "vehicle_configs")
PRESETS_DIR = os.path.join(VEHICLES_DIR, "common")
GENERIC_DIR = os.path.join(SCRATCH_DIR, "generic")

SOUND_TYPES = [
    "idle", "idling", "rev", "start", "knock", "turbo", "wastegate", "horn",
    "jakebrake", "fan", "siren", "airbrake", "parkingbrake",
    "shifting", "reversing", "indicator", "coupling", "supercharger",
    "uncoupling", "sound1", "tiresqueal", "hydraulicpump",
    "hydraulicflow", "trackrattle", "bucketrattle",
    "bell", "door", "scanner", "music", "whistle", "gun", "outoffuel", "others", "brake",
    "chevy", "cranking", "diesel", "engine", "hydraulic", "hydraulicFlow", "hydraulicPump",
    "hz", "psalm", "rattle", "roar", "squeal", "chirp"
]

def main():
    os.makedirs(VEHICLES_DIR, exist_ok=True)
    os.makedirs(PRESETS_DIR, exist_ok=True)
    os.makedirs(GENERIC_DIR, exist_ok=True)
    os.makedirs(os.path.join(PRESETS_DIR, "heavy_truck"), exist_ok=True)
    os.makedirs(os.path.join(PRESETS_DIR, "excavator"), exist_ok=True)
    os.makedirs(os.path.join(PRESETS_DIR, "locomotive"), exist_ok=True)

    files = [f for f in os.listdir(SCRATCH_DIR) if f.endswith('.json') and os.path.isfile(os.path.join(SCRATCH_DIR, f))]

    organized_count = 0

    for filename in files:
        filepath = os.path.join(SCRATCH_DIR, filename)
        
        matched_type = None
        for st in SOUND_TYPES:
            if filename.startswith(st + "-") or filename == st + ".json":
                matched_type = st
                break

        if not matched_type:
            # Place unmapped loose JSON into generic
            target = os.path.join(GENERIC_DIR, filename)
            shutil.move(filepath, target)
            organized_count += 1
            continue

        if filename == matched_type + ".json":
            target = os.path.join(GENERIC_DIR, filename)
            shutil.move(filepath, target)
            organized_count += 1
            continue

        remainder = filename[len(matched_type) + 1:-5]

        if matched_type in ["airbrake", "brake", "reversing", "shifting", "indicator", "coupling", "uncoupling"]:
            target = os.path.join(PRESETS_DIR, "heavy_truck", f"{matched_type}.json")
            if not os.path.exists(target):
                shutil.copy(filepath, target)
            veh_dir = os.path.join(VEHICLES_DIR, remainder, "sounds")
            os.makedirs(veh_dir, exist_ok=True)
            shutil.move(filepath, os.path.join(veh_dir, f"{matched_type}.json"))
            organized_count += 1
        elif matched_type in ["hydraulic", "hydraulicpump", "hydraulicflow", "hydraulicFlow", "hydraulicPump", "rattle", "bucketrattle"]:
            target = os.path.join(PRESETS_DIR, "excavator", f"{matched_type}.json")
            if not os.path.exists(target):
                shutil.copy(filepath, target)
            veh_dir = os.path.join(VEHICLES_DIR, remainder, "sounds")
            os.makedirs(veh_dir, exist_ok=True)
            shutil.move(filepath, os.path.join(veh_dir, f"{matched_type}.json"))
            organized_count += 1
        else:
            veh_dir = os.path.join(VEHICLES_DIR, remainder, "sounds")
            os.makedirs(veh_dir, exist_ok=True)
            shutil.move(filepath, os.path.join(veh_dir, f"{matched_type}.json"))
            organized_count += 1

    print(f"Successfully organized {organized_count} remaining sound files.")

if __name__ == "__main__":
    main()
