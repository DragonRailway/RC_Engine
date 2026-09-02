#!/usr/bin/env python3
"""
verify_remote_api_widgets.py
----------------------------
Automated verification of firmware-to-app widget synchronization using the
RadioKit Remote REST API (running on Android at 127.0.0.1:17007).

Tests:
1. Transmission Auto-Shift on Engine Start / Stop (Widgets 4 and 9)
2. Left Indicator Opposite Steering Auto-Cancel (Widgets 5 and 0)
3. Right Indicator Opposite Steering Auto-Cancel (Widgets 6 and 0)
4. Left Indicator Return-to-Center Auto-Cancel (Widgets 5 and 0)
5. Right Indicator Return-to-Center Auto-Cancel (Widgets 6 and 0)
6. Dedicated Lighting Controls (Widget 3)
"""

import sys
import time
import json
import urllib.request
import urllib.error

API_BASES = ["http://127.0.0.1:7007/api", "http://10.0.0.6:7007/api", "http://127.0.0.1:17007/api"]

def get_api_base():
    for base in API_BASES:
        try:
            req = urllib.request.Request(f"{base}/status")
            with urllib.request.urlopen(req, timeout=1.5) as resp:
                if resp.status == 200:
                    return base
        except Exception:
            continue
    return "http://127.0.0.1:7007/api"

API_BASE = get_api_base()

# Widget IDs
W_STEER      = 0  # Knob
W_GAS        = 1  # Slider
W_BRAKE      = 2  # Slider
W_TRUCK_LIGHT= 3  # MultipleSelect (Low=1, High=2, Fog=4)
W_START      = 4  # ToggleButton
W_LEFT_IND   = 5  # ToggleButton
W_RIGHT_IND  = 6  # ToggleButton
W_AUX        = 7  # Slider
W_HORN       = 8  # PushButton
W_GEAR       = 9  # MultipleButton (D=0, P=1, R=2)

def http_get(path):
    url = f"{API_BASE}{path}"
    req = urllib.request.Request(url, headers={"Accept": "application/json"})
    with urllib.request.urlopen(req, timeout=5.0) as resp:
        return json.loads(resp.read().decode("utf-8"))

def http_put(path, payload):
    url = f"{API_BASE}{path}"
    data = json.dumps(payload).encode("utf-8")
    req = urllib.request.Request(url, data=data, method="PUT", headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=5.0) as resp:
        return json.loads(resp.read().decode("utf-8"))

def http_post(path, payload=None):
    url = f"{API_BASE}{path}"
    data = json.dumps(payload).encode("utf-8") if payload else None
    req = urllib.request.Request(url, data=data, method="POST", headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=5.0) as resp:
        return json.loads(resp.read().decode("utf-8"))

def set_widget(wid, values):
    return http_put(f"/widgets/{wid}", {"values": values})

def get_widget_val(wid):
    data = http_get(f"/widgets/{wid}")
    return data.get("widget", {}).get("state", {}).get("value")

def wait_for_widget_val(wid, expected_val, timeout=3.0):
    start = time.time()
    last_val = None
    while time.time() - start < timeout:
        last_val = get_widget_val(wid)
        if last_val == expected_val:
            return True, last_val
        time.sleep(0.05)
    return False, last_val

def ensure_connected():
    status = http_get("/status")
    print(f"Connected to RadioKit API (App v{status.get('version')}, Platform: {status.get('platform')})")
    
    conn = http_get("/connection")
    for attempt in range(4):
        if conn.get("connected"):
            break
        print(f"BLE link inactive. Connecting to BLE device (attempt {attempt+1}/4)...")
        try:
            http_post("/pair/scan", {"type": "ble"})
            time.sleep(2.5)
            devs = http_get("/pair/devices").get("devices", [])
            if devs:
                try:
                    http_post("/connection/connect", {"id": devs[0]["id"], "type": "ble"})
                except Exception:
                    pass
                time.sleep(3.0)
                conn = http_get("/connection")
        except Exception:
            time.sleep(1.0)
            conn = http_get("/connection")
    
    if not conn.get("connected"):
        print("ERROR: Could not establish BLE connection")
        sys.exit(1)
    
    dev = conn.get("device", {})
    print(f"✓ BLE Connected to: '{dev.get('name')}' ({dev.get('id')}) | RSSI: {conn.get('rssi')} dBm\n")

def main():
    print("==================================================================")
    print("  RadioKit Remote REST API - Widget State Sync Verification Suite")
    print("==================================================================\n")
    
    ensure_connected()
    
    total_tests = 0
    passed_tests = 0
    
    # -------------------------------------------------------------
    # Test 1: Engine Stop -> Gear Auto-Shift to Park (P=1)
    # -------------------------------------------------------------
    total_tests += 1
    print("[Test 1] Engine Stop -> Gear Auto-Shift to Park (P=1)...")
    set_widget(W_START, [0]) # STOP
    ok, val = wait_for_widget_val(W_GEAR, 1, timeout=3.0)
    if ok:
        print(f"  ✓ PASS: gear_switch automatically updated to Park (P=1) in App UI [state.value={val}]")
        passed_tests += 1
    else:
        print(f"  ✗ FAIL: gear_switch did not update to Park (got state.value={val})")

    # -------------------------------------------------------------
    # Test 2: Engine Start -> Gear Auto-Shift to Drive (D=0)
    # -------------------------------------------------------------
    total_tests += 1
    print("\n[Test 2] Engine Start -> Gear Auto-Shift to Drive (D=0)...")
    set_widget(W_START, [1]) # START
    ok, val = wait_for_widget_val(W_GEAR, 0, timeout=3.0)
    if ok:
        print(f"  ✓ PASS: gear_switch automatically updated to Drive (D=0) in App UI [state.value={val}]")
        passed_tests += 1
    else:
        print(f"  ✗ FAIL: gear_switch did not update to Drive (got state.value={val})")

    # -------------------------------------------------------------
    # Test 3: Left Indicator Opposite Steering Cancellation
    # -------------------------------------------------------------
    total_tests += 1
    print("\n[Test 3] Left Indicator Opposite Steering Auto-Cancel...")
    set_widget(W_STEER, [0])
    time.sleep(0.2)
    set_widget(W_LEFT_IND, [1]) # Turn ON Left indicator
    ok, val = wait_for_widget_val(W_LEFT_IND, 1, timeout=2.0)
    assert ok, f"Left indicator failed to turn ON (val={val})"
    print(f"  - Left indicator turned ON [state.value={val}]")
    time.sleep(0.3)
    
    print("  - Turning steering wheel to the opposite side (+35% Right)...")
    set_widget(W_STEER, [35])
    ok, val = wait_for_widget_val(W_LEFT_IND, 0, timeout=2.5)
    if ok:
        print(f"  ✓ PASS: left_indicator automatically toggled OFF in App UI [state.value={val}]")
        passed_tests += 1
    else:
        print(f"  ✗ FAIL: left_indicator stayed ON (got state.value={val})")
    set_widget(W_STEER, [0])
    time.sleep(0.3)

    # -------------------------------------------------------------
    # Test 4: Right Indicator Opposite Steering Cancellation
    # -------------------------------------------------------------
    total_tests += 1
    print("\n[Test 4] Right Indicator Opposite Steering Auto-Cancel...")
    set_widget(W_STEER, [0])
    time.sleep(0.2)
    set_widget(W_RIGHT_IND, [1]) # Turn ON Right indicator
    ok, val = wait_for_widget_val(W_RIGHT_IND, 1, timeout=2.0)
    assert ok, f"Right indicator failed to turn ON (val={val})"
    print(f"  - Right indicator turned ON [state.value={val}]")
    time.sleep(0.3)
    
    print("  - Turning steering wheel to the opposite side (-35% Left)...")
    set_widget(W_STEER, [-35])
    ok, val = wait_for_widget_val(W_RIGHT_IND, 0, timeout=2.5)
    if ok:
        print(f"  ✓ PASS: right_indicator automatically toggled OFF in App UI [state.value={val}]")
        passed_tests += 1
    else:
        print(f"  ✗ FAIL: right_indicator stayed ON (got state.value={val})")
    set_widget(W_STEER, [0])
    time.sleep(0.3)

    # -------------------------------------------------------------
    # Test 5: Left Indicator Return-to-Center Cancellation
    # -------------------------------------------------------------
    total_tests += 1
    print("\n[Test 5] Left Indicator Return-to-Center Auto-Cancel...")
    set_widget(W_STEER, [0])
    time.sleep(0.2)
    set_widget(W_LEFT_IND, [1]) # Turn ON Left indicator
    ok, val = wait_for_widget_val(W_LEFT_IND, 1, timeout=2.0)
    print(f"  - Left indicator ON [state.value={val}]")
    time.sleep(0.3)
    
    print("  - Steering into turn (-50% Left)...")
    set_widget(W_STEER, [-50])
    time.sleep(0.4)
    val = get_widget_val(W_LEFT_IND)
    print(f"  - Left indicator remains active in turn [state.value={val}]")
    
    print("  - Returning steering wheel to center (0%)...")
    set_widget(W_STEER, [0])
    ok, val = wait_for_widget_val(W_LEFT_IND, 0, timeout=2.5)
    if ok:
        print(f"  ✓ PASS: left_indicator automatically toggled OFF in App UI on center return [state.value={val}]")
        passed_tests += 1
    else:
        print(f"  ✗ FAIL: left_indicator did not cancel on return (got state.value={val})")
    set_widget(W_STEER, [0])
    time.sleep(0.3)

    # -------------------------------------------------------------
    # Test 6: Right Indicator Return-to-Center Cancellation
    # -------------------------------------------------------------
    total_tests += 1
    print("\n[Test 6] Right Indicator Return-to-Center Auto-Cancel...")
    set_widget(W_STEER, [0])
    time.sleep(0.2)
    set_widget(W_RIGHT_IND, [1]) # Turn ON Right indicator
    ok, val = wait_for_widget_val(W_RIGHT_IND, 1, timeout=2.0)
    print(f"  - Right indicator ON [state.value={val}]")
    time.sleep(0.3)
    
    print("  - Steering into turn (+50% Right)...")
    set_widget(W_STEER, [50])
    time.sleep(0.4)
    val = get_widget_val(W_RIGHT_IND)
    print(f"  - Right indicator remains active in turn [state.value={val}]")
    
    print("  - Returning steering wheel to center (0%)...")
    set_widget(W_STEER, [0])
    ok, val = wait_for_widget_val(W_RIGHT_IND, 0, timeout=2.5)
    if ok:
        print(f"  ✓ PASS: right_indicator automatically cancelled on wheel return [state.value={val}]")
        passed_tests += 1
    else:
        print(f"  ✗ FAIL: right_indicator did not cancel on return (got state.value={val})")
    time.sleep(0.2)

    # -------------------------------------------------------------
    # Test 7: Dedicated Lighting Mode Verification via Remote API
    # -------------------------------------------------------------
    total_tests += 1
    print("\n[Test 7] Dedicated Lighting: Low, High, Fog, and Off...")
    # Low Beam
    set_widget(W_TRUCK_LIGHT, [1])
    ok_low, val_low = wait_for_widget_val(W_TRUCK_LIGHT, 1, timeout=1.5)
    # High Beam
    set_widget(W_TRUCK_LIGHT, [2])
    ok_high, val_high = wait_for_widget_val(W_TRUCK_LIGHT, 2, timeout=1.5)
    # Fog Lamp
    set_widget(W_TRUCK_LIGHT, [4])
    ok_fog, val_fog = wait_for_widget_val(W_TRUCK_LIGHT, 4, timeout=1.5)
    # All Off
    set_widget(W_TRUCK_LIGHT, [0])
    ok_off, val_off = wait_for_widget_val(W_TRUCK_LIGHT, 0, timeout=1.5)

    if ok_low and ok_high and ok_fog and ok_off:
        print(f"  ✓ PASS: All dedicated lighting states verified in App UI (Low=1, High=2, Fog=4, Off=0)")
        passed_tests += 1
    else:
        print(f"  ✗ FAIL: Lighting states: low={ok_low}, high={ok_high}, fog={ok_fog}, off={ok_off}")

    # -------------------------------------------------------------
    # Test 8: Relative Steering Activation (Wheel already turned +30% -> Enable Left Indicator)
    # -------------------------------------------------------------
    total_tests += 1
    print("\n[Test 8] Relative Steering: Enable Left Indicator while wheel is at +30%...")
    set_widget(W_STEER, [30])
    time.sleep(0.2)
    set_widget(W_LEFT_IND, [1])
    time.sleep(0.3)
    val = get_widget_val(W_LEFT_IND)
    if val == 1:
        print(f"  ✓ PASS: Left indicator successfully enabled and stayed active at +30% baseline [state.value={val}]")
        # Steer right to +50% (delta = +20 > 15) -> cancels
        set_widget(W_STEER, [50])
        ok, val = wait_for_widget_val(W_LEFT_IND, 0, timeout=2.0)
        if ok:
            print(f"  ✓ PASS: Auto-cancelled when steered right to +50% (delta > +15) [state.value={val}]")
            passed_tests += 1
        else:
            print(f"  ✗ FAIL: Did not auto-cancel on opposite steer (got state.value={val})")
    else:
        print(f"  ✗ FAIL: Could not enable indicator with wheel turned (got state.value={val})")
    set_widget(W_STEER, [0])
    time.sleep(0.2)

    # -------------------------------------------------------------
    # Test 9: Relative Steering Activation (Wheel already turned -30% -> Enable Right Indicator)
    # -------------------------------------------------------------
    total_tests += 1
    print("\n[Test 9] Relative Steering: Enable Right Indicator while wheel is at -30%...")
    set_widget(W_STEER, [-30])
    time.sleep(0.2)
    set_widget(W_RIGHT_IND, [1])
    time.sleep(0.3)
    val = get_widget_val(W_RIGHT_IND)
    if val == 1:
        print(f"  ✓ PASS: Right indicator successfully enabled and stayed active at -30% baseline [state.value={val}]")
        # Steer left to -50% (delta = -20 < -15) -> cancels
        set_widget(W_STEER, [-50])
        ok, val = wait_for_widget_val(W_RIGHT_IND, 0, timeout=2.0)
        if ok:
            print(f"  ✓ PASS: Auto-cancelled when steered left to -50% (delta < -15) [state.value={val}]")
            passed_tests += 1
        else:
            print(f"  ✗ FAIL: Did not auto-cancel on opposite steer (got state.value={val})")
    else:
        print(f"  ✗ FAIL: Could not enable indicator with wheel turned (got state.value={val})")
    set_widget(W_STEER, [0])
    time.sleep(0.2)

    print("\n" + "="*66)
    if passed_tests == total_tests:
        print(f"  ✓ ALL {passed_tests}/{total_tests} REMOTE API WIDGET SYNC TESTS PASSED!")
    else:
        print(f"  ✗ {passed_tests}/{total_tests} TESTS PASSED ({total_tests - passed_tests} FAILED)")
    print("="*66 + "\n")

    if passed_tests != total_tests:
        sys.exit(1)

if __name__ == "__main__":
    main()
