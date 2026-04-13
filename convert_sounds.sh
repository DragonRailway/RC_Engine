#!/bin/bash

# Configuration
SOURCE_DIR="lib/RcEngineSound/src/vehicles/sounds"
OUTPUT_DIR="$SOURCE_DIR/json"

# Clear previous output directory to ensure clean prefixing
rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

# Sound Type Keywords (ordered for priority/specificity)
KEYWORDS=("jakeBrake" "wastegate" "reversing" "indicator" "coupling" "hydraulic" "trackRattle" "cranking" "shfiting" "whistle" "indicator" "idle" "rev" "start" "horn" "siren" "knock" "brake" "fan" "turbo" "chirp" "door" "roar")

echo "Starting conversion of headers to JSON with prefixed types..."

for f in "$SOURCE_DIR"/*.h; do
    [ -e "$f" ] || continue
    original_name=$(basename "$f" .h)
    
    # Defaults
    type=""
    base_name="$original_name"
    
    # Detect type
    for kw in "${KEYWORDS[@]}"; do
        # Use grep to find if the keyword exists (case insensitive)
        actual_kw=$(echo "$original_name" | grep -ioE "$kw")
        if [ ! -z "$actual_kw" ]; then
            # Use lowercase for the prefix type
            type=$(echo "$kw" | tr '[:upper:]' '[:lower:]')
            # Remove the found keyword from the original name to get the remainder
            # We use sed with the 'i' flag for case insensitivity
            base_name=$(echo "$original_name" | sed -E "s/$kw//i")
            break
        fi
    done
    
    # Final cleanup of base name: strip underscores/dashes and handle "unknown"
    base_name=$(echo "$base_name" | sed 's/^_//;s/_$//;s/^-//;s/-$//')
    
    if [ -z "$type" ]; then
        target_name="${base_name}.json"
    else
        target_name="${type}-${base_name}.json"
    fi
    
    # Extract values
    rate=$(grep -i "sampleRate" "$f" | grep -oE "[0-9]+" | head -n 1)
    count=$(grep -i "sampleCount" "$f" | grep -oE "[0-9]+" | head -n 1)

    # Extract the array content
    samples=$(sed -n '/{/,/};/p' "$f" | \
              sed 's/\/\/.*//' | \
              tr -d '\r\n[:space:]' | \
              sed 's/.*{//' | \
              sed 's/};.*//' | \
              sed 's/,$//')

    # Generate JSON
    echo "{\"sampleRate\": ${rate:-0}, \"sampleCount\": ${count:-0}, \"samples\": [${samples}]}" > "$OUTPUT_DIR/${target_name}"
    
    echo "Generated: $target_name"
done

echo "Done! Prefixed JSON files are in $OUTPUT_DIR"
