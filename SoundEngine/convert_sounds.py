#!/usr/bin/env python3
"""
Convert C header sound files to JSON format.

Scans raw_sounds/*.h for PCM audio data and outputs
JSON files to sounds/ directory.

Usage:
    python3 convert_sounds.py
"""

import json
import os
import re
import shutil
import sys
from pathlib import Path

# Paths relative to this script's directory
SCRIPT_DIR = Path(__file__).parent.resolve()
SOURCE_DIR = SCRIPT_DIR / "raw_sounds"
OUTPUT_DIR = SCRIPT_DIR / "sounds"

# Sound type keywords (order matters for priority matching)
# Types: engine sounds, effects, music, voice, test tones
KEYWORDS = [
    # Engine types (specific first)
    "jakeBrake", "wastegate", "shifting", "cranking",
    "idle", "idling", "rev", "knock", "brake",
    "fan", "turbo", "supercharger", "chirp", "roar",
    # Horn/siren
    "horn", "bell", "siren", "scanner", "whistle",
    # Effects
    "reversing", "indicator", "coupling", "hydraulic",
    "trackRattle", "door", "rattle", "squeal", "gun",
    # Voice clips
    "OutOfFuel",
    # Music (broad keywords)
    "music", "psalm", "anthem", "hymn",
    # Engine variants
    "diesel", "chevy", "engine",
    # Test tones
    "Hz",
    # Last resort engine types (prefix-heavy, match last)
    "start",
]

# Override mapping for files that don't have category keywords in name
# Maps filename stem -> type prefix
OVERRIDE_TYPES = {
    # Music tracks
    "007JamesBond": "music",
    "Bond": "music",
    "Pigs": "music",
    "Tequila(1)": "music",
    "brasil(1)": "music",
    "in_the_summertime(1)": "music",
    "la_carica(1)": "music",
    "la_cucaracha(1)": "music",
    "marseillaise(1)": "music",
    "river_kwai(1)": "music",
    "susannah(1)": "music",
    "startrek(1)": "music",
    "GlenCanyon": "music",
    # Voice clips
    "MesserschmittBf109Guns": "gun",
    "Tankmotorsound": "engine",
}


def clean_output_dir():
    """Remove all files in the output directory."""
    if OUTPUT_DIR.exists():
        for f in OUTPUT_DIR.iterdir():
            if f.is_file():
                f.unlink()
        print(f"Cleaned: {OUTPUT_DIR}")
    else:
        OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
        print(f"Created: {OUTPUT_DIR}")


def detect_type(filename):
    """Detect sound type from filename using keyword matching.

    Returns (type, base_name) tuple.
    type is lowercase keyword if found, empty string otherwise.
    base_name is the filename with the keyword stripped.
    """
    name = filename.stem  # without .h extension

    # Check override mapping first
    if name in OVERRIDE_TYPES:
        clean = re.sub(r'\(\d+\)', '', name)
        clean = re.sub(r'^[_\-]+|[_\-]+$', '', clean)
        return OVERRIDE_TYPES[name], clean

    best_match = None
    best_kw = None

    for kw in KEYWORDS:
        # Use negative lookahead for "start" to avoid matching "startrek"
        if kw == "start":
            pattern = r'start(?!rek)'
        else:
            pattern = kw
        match = re.search(pattern, name, re.IGNORECASE)
        if match:
            # Prefer the match that ends latest in the string
            if best_match is None or match.end() > best_match.end():
                best_match = match
                best_kw = kw

    if best_match:
        sound_type = best_kw.lower()
        # Remove the matched keyword from the name
        base = name[:best_match.start()] + name[best_match.end():]
        # Strip parenthesized numbers like "(1)"
        base = re.sub(r'\(\d+\)', '', base)
        # Strip leading/trailing underscores, dashes
        base = re.sub(r'^[_\-]+|[_\-]+$', '', base)
        return sound_type, base

    return "", name


def extract_header_data(content):
    """Extract sampleRate, sampleCount, and samples array from header content."""
    # Extract sampleRate
    rate_match = re.search(r'sampleRate\s*=\s*(\d+)', content, re.IGNORECASE)
    rate = int(rate_match.group(1)) if rate_match else 0

    # Extract sampleCount
    count_match = re.search(r'sampleCount\s*=\s*(\d+)', content, re.IGNORECASE)
    count = int(count_match.group(1)) if count_match else 0

    # Extract samples array content between { and }
    # Find the array declaration (samples[], hornSamples[], etc.)
    array_match = re.search(
        r'(?:samples|hornSamples|sirenSamples)\[\]\s*=\s*\{([^}]+)\}',
        content, re.IGNORECASE
    )

    if array_match:
        raw = array_match.group(1)
        # Strip comments (// to end of line)
        raw = re.sub(r'//.*', '', raw)
        # Strip whitespace and newlines
        raw = re.sub(r'\s+', '', raw)
        # Remove trailing comma
        raw = raw.rstrip(',')
        # Parse numbers
        samples = [int(x.strip()) for x in raw.split(',') if x.strip()]
    else:
        samples = []

    return rate, count, samples


def convert_file(h_file):
    """Convert a single .h file to JSON."""
    content = h_file.read_text(encoding='utf-8', errors='replace')
    rate, count, samples = extract_header_data(content)

    sound_type, base_name = detect_type(h_file)

    if sound_type and base_name:
        out_name = f"{sound_type}-{base_name}.json"
    elif sound_type:
        out_name = f"{sound_type}.json"
    elif base_name:
        out_name = f"others-{base_name}.json"
    else:
        out_name = "others.json"

    data = {
        "sampleRate": rate,
        "sampleCount": count,
        "samples": samples
    }

    out_path = OUTPUT_DIR / out_name
    out_path.write_text(json.dumps(data), encoding='utf-8')

    return out_name


def main():
    print(f"Source: {SOURCE_DIR}")
    print(f"Output: {OUTPUT_DIR}")
    print()

    if not SOURCE_DIR.exists():
        print(f"Error: Source directory not found: {SOURCE_DIR}")
        sys.exit(1)

    # Clean output directory first
    clean_output_dir()

    # Find all .h files
    h_files = sorted(SOURCE_DIR.glob("*.h"))
    if not h_files:
        print(f"No .h files found in {SOURCE_DIR}")
        sys.exit(1)

    print(f"Converting {len(h_files)} files...")
    print()

    for h_file in h_files:
        out_name = convert_file(h_file)
        print(f"  {h_file.name} -> {out_name}")

    print()
    print(f"Done! {len(h_files)} JSON files in {OUTPUT_DIR}")


if __name__ == "__main__":
    main()
