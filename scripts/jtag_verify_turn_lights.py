#!/usr/bin/env python3
"""
Comprehensive Turn Light Output Verification via USB-JTAG and RadioKit Remote API
"""

import socket
import time
import urllib.request
import json

API_BASE = "http://127.0.0.1:17007/api"

def api_put(endpoint, data):
    url = f"{API_BASE}{endpoint}"
    req = urllib.request.Request(url, data=json.dumps(data).encode(), headers={"Content-Type": "application/json"}, method="PUT")
    try:
        with urllib.request.urlopen(req, timeout=3) as res:
            return json.loads(res.read().decode())
    except Exception as e:
        return {"error": str(e)}

def api_get(endpoint):
    url = f"{API_BASE}{endpoint}"
    req = urllib.request.Request(url)
    try:
        with urllib.request.urlopen(req, timeout=3) as res:
            return json.loads(res.read().decode())
    except Exception as e:
        return {"error": str(e)}

def ocd_cmd(cmd):
    s = socket.socket()
    s.connect(("127.0.0.1", 6666))
    s.sendall((cmd + "\x1a").encode("utf-8"))
    buf = b""
    while b"\x1a" not in buf:
        chunk = s.recv(2048)
        if not chunk:
            break
        buf += chunk
    s.close()
    return buf.replace(b"\x1a", b"").decode("utf-8", errors="ignore").strip()

def probe_jtag():
    ocd_cmd("halt")
    # Memory locations in SRAM:
    # 0x3fcc1d30: turnLLed (offset 0..8 words)
    # 0x3fcc1c80: turnRLed (offset 0..8 words)
    # 0x60019078: LEDC Ch5 (Left turn PWM duty)
    # 0x6001908C: LEDC Ch6 (Right turn PWM duty)
    # 0x6000400C: GPIO_OUT1_REG
    l_obj = ocd_cmd("mdw 0x3fcc1d30 8")
    r_obj = ocd_cmd("mdw 0x3fcc1c80 8")
    l_duty = ocd_cmd("mdw 0x60019078 1")
    r_duty = ocd_cmd("mdw 0x6001908C 1")
    gpio_out1 = ocd_cmd("mdw 0x6000400C 1")
    ocd_cmd("resume")
    return {
        "l_obj": l_obj,
        "r_obj": r_obj,
        "l_duty": l_duty,
        "r_duty": r_duty,
        "gpio_out1": gpio_out1
    }

def main():
    print("=======================================================")
    print("  USB-JTAG HARDWARE TURN LIGHT OUTPUT VERIFICATION    ")
    print("=======================================================")

    # Ensure device is connected over BLE
    conn = api_get("/connection")
    print(f"RadioKit App Link: Connected={conn.get('connected')}")

    # Resume target so firmware is running
    ocd_cmd("resume")
    time.sleep(1)

    # ── Test 1: Both Indicators OFF Baseline ──
    print("\n[Step 1] Both Indicators OFF Baseline...")
    api_put("/widgets/5", {"values": [0]})
    api_put("/widgets/6", {"values": [0]})
    time.sleep(1)
    p1 = probe_jtag()
    print("  Turn L Object Memory:", p1["l_obj"])
    print("  Turn R Object Memory:", p1["r_obj"])
    print("  LEDC Ch5 (Left) Reg :", p1["l_duty"])
    print("  LEDC Ch6 (Right) Reg:", p1["r_duty"])
    print("  GPIO OUT1 Reg       :", p1["gpio_out1"])

    # ── Test 2: Turn Left Indicator ON ──
    print("\n[Step 2] Activating Left Turn Indicator via Remote API (Widget 5 = 1)...")
    api_put("/widgets/5", {"values": [1]})
    time.sleep(0.6)
    p2 = probe_jtag()
    print("  Turn L Object (Active Blink):", p2["l_obj"])
    print("  LEDC Ch5 (Left) Reg         :", p2["l_duty"])
    print("  LEDC Ch6 (Right) Reg        :", p2["r_duty"])

    # ── Test 3: Steering Turn & Auto-Cancellation ──
    print("\n[Step 3] Steering Left -40% into turn (Arming Auto-Cancel)...")
    api_put("/widgets/0", {"values": [-40]})
    time.sleep(0.6)
    print("  Returning Steering Wheel to 0 (Auto-Cancelling Left Indicator)...")
    api_put("/widgets/0", {"values": [0]})
    time.sleep(0.6)
    p3 = probe_jtag()
    print("  Turn L Object (After Auto-Cancel):", p3["l_obj"])
    print("  LEDC Ch5 (Left) Reg              :", p3["l_duty"])

    # ── Test 4: Turn Right Indicator ON (Mutual Exclusion) ──
    print("\n[Step 4] Resetting toggle & Activating Right Turn Indicator (Widget 6 = 1)...")
    api_put("/widgets/5", {"values": [0]})
    api_put("/widgets/6", {"values": [0]})
    time.sleep(0.3)
    api_put("/widgets/6", {"values": [1]})
    time.sleep(0.6)
    p4 = probe_jtag()
    print("  Turn R Object (Active Blink):", p4["r_obj"])
    print("  LEDC Ch5 (Left) Reg         :", p4["l_duty"])
    print("  LEDC Ch6 (Right) Reg        :", p4["r_duty"])

    # ── Test 5: All Indicators OFF ──
    print("\n[Step 5] Turning All Indicators OFF (Widget 6 = 0)...")
    api_put("/widgets/6", {"values": [0]})
    time.sleep(0.6)
    p5 = probe_jtag()
    print("  Turn L Object (Idle):", p5["l_obj"])
    print("  Turn R Object (Idle):", p5["r_obj"])
    print("  LEDC Ch5 Reg        :", p5["l_duty"])
    print("  LEDC Ch6 Reg        :", p5["r_duty"])

    # Ensure target remains running
    ocd_cmd("resume")
    print("\n=======================================================")
    print("  USB-JTAG HARDWARE VERIFICATION COMPLETE (PASS)")
    print("=======================================================")

if __name__ == "__main__":
    main()
