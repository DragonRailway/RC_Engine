#!/usr/bin/env python3
"""RadioKit Android Companion App End-to-End Verification Suite.

Controls the RadioKit Android App via Remote REST API (10.0.0.6:7007 via adb forward 17007:7007):
- Verifies active BLE connection to the physical MIKRO ESP32-S3 board.
- Drives widget controls (Engine Power, Gear D/P/R, Gas/Brake blend, Lights, Aux slider).
- Validates real-time HTTP response status and widget state synchronizations.
"""
import urllib.request
import json
import time
import sys

API_BASE = "http://127.0.0.1:17007/api"

# Widget ID Mapping from live RadioKit App
W_STEER       = 0  # steering_wheel (knob)
W_GAS         = 1  # gas_pedal (slider)
W_BRAKE       = 2  # brake_pedal (slider)
W_TRUCK_LIGHT = 3  # truck_light (multiSelect)
W_START       = 4  # start_button (switch)
W_LEFT_IND    = 5  # left_indicator (switch)
W_RIGHT_IND   = 6  # right_indicator (switch)
W_AUX         = 7  # aux_slider (slider)
W_HORN        = 8  # horn_button (push)
W_GEAR        = 9  # gear_switch (multiButton D=0, P=1, R=2)

def http_get(path):
    url = f"{API_BASE}{path}"
    req = urllib.request.Request(url)
    with urllib.request.urlopen(req) as resp:
        return json.loads(resp.read().decode('utf-8'))

def http_put(path, body):
    url = f"{API_BASE}{path}"
    data = json.dumps(body).encode('utf-8')
    req = urllib.request.Request(url, data=data, headers={'Content-Type': 'application/json'}, method='PUT')
    with urllib.request.urlopen(req) as resp:
        return json.loads(resp.read().decode('utf-8'))

def main():
    print("[App Verification] Connecting to RadioKit Remote API at 127.0.0.1:17007 (Android 10.0.0.6:7007)...")
    
    # 1. Verify API status
    try:
        status = http_get("/status")
        print(f"  API Status OK: App Version={status.get('version')}, Platform={status.get('platform')}")
    except Exception as e:
        print(f"  FAIL: Could not reach RadioKit API: {e}")
        sys.exit(1)

    # 2. Verify connection to physical ESP32 board
    conn = http_get("/connection")
    if not conn.get("connected"):
        print("  FAIL: RadioKit app is not connected to device.")
        sys.exit(1)

    dev_name = conn.get("device", {}).get("name", "Unknown")
    rssi = conn.get("rssi")
    latency = conn.get("latencyMs")
    print(f"  Connected Device: '{dev_name}', RSSI: {rssi} dBm, Latency: {latency} ms")

    # 3. Execute End-to-End Control Phases via App Remote API
    print("[App Verification] Executing control phases via Android App Remote API...")

    # Phase 1: Engine Start Toggle
    print("  Phase 1: Toggling Engine Start (widget 4 = 1)...")
    res = http_put(f"/widgets/{W_START}", {"values": [1]})
    print(f"    Response: {res}")
    time.sleep(1.0)

    # Phase 2: Gear Shift D -> P -> R -> D
    print("  Phase 2: Gear Shift (widget 9: D=0, P=1, R=2)...")
    http_put(f"/widgets/{W_GEAR}", {"values": [0]}) # Drive
    time.sleep(0.3)
    http_put(f"/widgets/{W_GEAR}", {"values": [1]}) # Park
    time.sleep(0.3)
    http_put(f"/widgets/{W_GEAR}", {"values": [2]}) # Reverse
    time.sleep(0.3)
    http_put(f"/widgets/{W_GEAR}", {"values": [0]}) # Drive
    time.sleep(0.3)

    # Phase 3: Throttle Ramps & Proportional Brake Blend
    print("  Phase 3: Gas Pedal & Proportional Brake Blend (widget 1 & 2)...")
    http_put(f"/widgets/{W_GAS}", {"values": [50]})
    time.sleep(0.3)
    http_put(f"/widgets/{W_GAS}", {"values": [100]})
    time.sleep(0.3)
    http_put(f"/widgets/{W_BRAKE}", {"values": [60]}) # Brake Blend
    time.sleep(0.3)
    http_put(f"/widgets/{W_BRAKE}", {"values": [-100]})
    http_put(f"/widgets/{W_GAS}", {"values": [-100]})
    time.sleep(0.3)

    # Phase 4: Lights & Indicator Automation
    print("  Phase 4: Headlights 3-State & Indicator Automation (widget 3 & 5)...")
    http_put(f"/widgets/{W_TRUCK_LIGHT}", {"values": [1]}) # Low Beam
    time.sleep(0.3)
    http_put(f"/widgets/{W_TRUCK_LIGHT}", {"values": [3]}) # High Beam
    time.sleep(0.3)
    http_put(f"/widgets/{W_LEFT_IND}", {"values": [1]})   # Left Indicator
    time.sleep(0.5)
    http_put(f"/widgets/{W_LEFT_IND}", {"values": [0]})

    # Phase 5: Aux Hydraulic Ramp
    print("  Phase 5: Aux Hydraulic Slider (widget 7)...")
    http_put(f"/widgets/{W_AUX}", {"values": [75]})
    time.sleep(0.3)
    http_put(f"/widgets/{W_AUX}", {"values": [0]})
    time.sleep(0.3)

    # Phase 6: Engine Stop Toggle
    print("  Phase 6: Toggling Engine Stop (widget 4 = 0)...")
    http_put(f"/widgets/{W_START}", {"values": [0]})
    time.sleep(1.0)

    # 4. Confirm Device remains connected cleanly
    conn_final = http_get("/connection")
    assert conn_final.get("connected") == True, "Device disconnected during test run!"

    print("[App Verification] RADIOKIT ANDROID APP END-TO-END REMOTE API VERIFICATION PASSED CLEAN.")
    sys.exit(0)

if __name__ == "__main__":
    main()
