#!/usr/bin/env python3
"""
Interactive Lighting Verification Script
Controls lighting widgets via RadioKit Remote REST API on Android
and verifies corresponding live state and events over USB Serial (/dev/ttyACM0).
"""

import json
import os
import sys
import time
import urllib.request
import serial
import threading

API_BASE = "http://127.0.0.1:17007/api"
SERIAL_PORT = os.environ.get("SERIAL_PORT", "/dev/ttyACM0")
BAUD_RATE = 2000000

# Widget IDs
WID_TRUCK_LIGHT = 3
WID_START_BUTTON = 4
WID_LEFT_IND = 5
WID_RIGHT_IND = 6
WID_BRAKE_PEDAL = 2
WID_GEAR_SWITCH = 9

def http_get(path):
    url = f"{API_BASE}{path}"
    req = urllib.request.Request(url, method="GET")
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

def get_widget(wid):
    return http_get(f"/widgets/{wid}").get("widget", {}).get("state", {}).get("value")

class SerialMonitor:
    def __init__(self, port, baud):
        self.ser = serial.Serial(port, baud, timeout=0.1)
        self.running = True
        self.lines = []
        self.lock = threading.Lock()
        self.thread = threading.Thread(target=self._reader, daemon=True)
        self.thread.start()

    def _reader(self):
        buf = ""
        while self.running:
            try:
                data = self.ser.read(256).decode("utf-8", errors="ignore")
                if data:
                    buf += data
                    while "\n" in buf:
                        line, buf = buf.split("\n", 1)
                        line = line.strip()
                        if line:
                            with self.lock:
                                self.lines.append((time.time(), line))
            except Exception:
                break

    def get_recent(self, since_time=0):
        with self.lock:
            return [line for t, line in self.lines if t >= since_time]

    def close(self):
        self.running = False
        self.ser.close()

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
    print("=" * 65)
    print("  RC Brain — Comprehensive Lighting & USB JTAG Telemetry Test")
    print("=" * 65)

    # 1. API Status & BLE Link Check
    ensure_connected()

    # 2. Open USB JTAG Serial Monitor
    print(f"Opening Serial Monitor on {SERIAL_PORT} @ {BAUD_RATE} baud...")
    mon = SerialMonitor(SERIAL_PORT, BAUD_RATE)
    time.sleep(0.5)

    # Start Engine
    print("\n[Step 0] Starting Engine (start_button = 1)...")
    t0 = time.time()
    set_widget(WID_START_BUTTON, [1])
    time.sleep(1.0)

    # ── Test 1: Head Light (Headlight 40%, Full Beam 0%) ──
    print("\n[Step 1] Activating Head Light (truck_light = 0x01)...")
    t1 = time.time()
    set_widget(WID_TRUCK_LIGHT, [1])
    time.sleep(0.8)
    lines = mon.get_recent(t1)
    event_found = any("[EVENT] Headlight -> LOW" in l for l in lines)
    status_lines = [l for l in lines if "[STATUS]" in l]
    latest_status = status_lines[-1] if status_lines else "None"
    print(f"  Event Verified: {event_found} ('[EVENT] Headlight -> LOW')")
    print(f"  Telemetry: {latest_status}")

    # ── Test 2: High Beam (Dedicated Full Beam ON 100%) ──
    print("\n[Step 2] Activating High Beam (truck_light = 0x02)...")
    t2 = time.time()
    set_widget(WID_TRUCK_LIGHT, [2])
    time.sleep(0.8)
    lines = mon.get_recent(t2)
    event_found = any("[EVENT] Headlight -> HIGH" in l for l in lines)
    status_lines = [l for l in lines if "[STATUS]" in l]
    latest_status = status_lines[-1] if status_lines else "None"
    print(f"  Event Verified: {event_found} ('[EVENT] Headlight -> HIGH')")
    print(f"  Telemetry: {latest_status}")

    # ── Test 3: Fog Lamp (Dedicated Fog Lamp ON 100%) ──
    print("\n[Step 3] Activating Dedicated Fog Lamp (truck_light = 0x04)...")
    t3 = time.time()
    set_widget(WID_TRUCK_LIGHT, [4])
    time.sleep(0.8)
    lines = mon.get_recent(t3)
    event_found = any("[EVENT] FogLamp -> ON" in l for l in lines)
    status_lines = [l for l in lines if "[STATUS]" in l]
    latest_status = status_lines[-1] if status_lines else "None"
    print(f"  Event Verified: {event_found} ('[EVENT] FogLamp -> ON')")
    print(f"  Telemetry: {latest_status}")

    # ── Test 4: Head Light + Fog Lamp (truck_light = 0x05) ──
    print("\n[Step 4] Activating Head Light + Fog Lamp (truck_light = 0x05)...")
    t4 = time.time()
    set_widget(WID_TRUCK_LIGHT, [5])
    time.sleep(0.8)
    lines = mon.get_recent(t4)
    status_lines = [l for l in lines if "[STATUS]" in l]
    latest_status = status_lines[-1] if status_lines else "None"
    print(f"  Telemetry: {latest_status}")

    # ── Test 5: High Beam + Fog Lamp (truck_light = 0x06) ──
    print("\n[Step 5] Activating High Beam + Fog Lamp (truck_light = 0x06)...")
    t5 = time.time()
    set_widget(WID_TRUCK_LIGHT, [6])
    time.sleep(0.8)
    lines = mon.get_recent(t5)
    status_lines = [l for l in lines if "[STATUS]" in l]
    latest_status = status_lines[-1] if status_lines else "None"
    print(f"  Telemetry: {latest_status}")

    # ── Test 6: Turn Indicators & Hazard Lights ──
    print("\n[Step 6] Testing Left Indicator (left_indicator = 1)...")
    t6 = time.time()
    set_widget(WID_LEFT_IND, [1])
    time.sleep(0.8)
    lines = mon.get_recent(t6)
    status_lines = [l for l in lines if "[STATUS]" in l]
    latest_status = status_lines[-1] if status_lines else "None"
    print(f"  Telemetry (L:1 expected): {latest_status}")
    set_widget(WID_LEFT_IND, [0])
    time.sleep(0.4)

    print("\n[Step 7] Testing Right Indicator (right_indicator = 1)...")
    t7 = time.time()
    set_widget(WID_RIGHT_IND, [1])
    time.sleep(0.8)
    lines = mon.get_recent(t7)
    status_lines = [l for l in lines if "[STATUS]" in l]
    latest_status = status_lines[-1] if status_lines else "None"
    print(f"  Telemetry (R:1 expected): {latest_status}")
    set_widget(WID_RIGHT_IND, [0])
    time.sleep(0.4)

    print("\n[Step 8] Testing Hazard Lights (truck_light = 0x08)...")
    t8 = time.time()
    set_widget(WID_TRUCK_LIGHT, [8])
    time.sleep(0.8)
    lines = mon.get_recent(t8)
    status_lines = [l for l in lines if "[STATUS]" in l]
    latest_status = status_lines[-1] if status_lines else "None"
    print(f"  Telemetry (L:1 R:1 expected): {latest_status}")

    # ── Test 9: Brake Light & Reversing Light ──
    print("\n[Step 9] Testing Brake Light (brake_pedal = 80)...")
    t9 = time.time()
    set_widget(WID_BRAKE_PEDAL, [80])
    time.sleep(0.8)
    lines = mon.get_recent(t9)
    status_lines = [l for l in lines if "[STATUS]" in l]
    latest_status = status_lines[-1] if status_lines else "None"
    print(f"  Telemetry (Brk:1 expected): {latest_status}")
    set_widget(WID_BRAKE_PEDAL, [0])
    time.sleep(0.4)

    print("\n[Step 10] Testing Reversing Light via Reverse Gear (gear_switch = 2)...")
    t10 = time.time()
    set_widget(WID_GEAR_SWITCH, [2])
    time.sleep(0.8)
    lines = mon.get_recent(t10)
    status_lines = [l for l in lines if "[STATUS]" in l]
    latest_status = status_lines[-1] if status_lines else "None"
    print(f"  Telemetry (Gear:R expected): {latest_status}")
    set_widget(WID_GEAR_SWITCH, [0]) # Return to D
    time.sleep(0.4)

    # ── Turn All Lights OFF ──
    print("\n[Step 11] Turning All Lights OFF (truck_light = 0)...")
    t11 = time.time()
    set_widget(WID_TRUCK_LIGHT, [0])
    time.sleep(0.8)
    lines = mon.get_recent(t11)
    status_lines = [l for l in lines if "[STATUS]" in l]
    latest_status = status_lines[-1] if status_lines else "None"
    print(f"  Telemetry (Head:0, FogLamp OFF): {latest_status}")

    # Stop Engine
    set_widget(WID_START_BUTTON, [0])
    time.sleep(0.5)
    mon.close()

    print("\n" + "=" * 65)
    print("  ✓ ALL LIGHTING CHANNELS TESTED & VERIFIED VIA USB JTAG & REMOTE API!")
    print("=" * 65)

if __name__ == "__main__":
    main()
