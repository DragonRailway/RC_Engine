#!/usr/bin/env python3
"""
Verify Turn Light Outputs via USB JTAG (OpenOCD + GDB)
Attaches to live ESP32-S3 via builtin JTAG, reads hardware registers and firmware objects:
- HardwareInit::turnLLed (GPIO 41, LEDC Ch 5)
- HardwareInit::turnRLed (GPIO 42, LEDC Ch 6)
- LEDC registers and GPIO output states
- Controls left/right indicators via RadioKit Remote API
"""

import subprocess
import time
import urllib.request
import json
import os
import signal

API_BASE = "http://127.0.0.1:17007/api"
GDB_BIN = "/home/sun/sandbox/fedora/.platformio/packages/tool-xtensa-esp-elf-gdb/bin/xtensa-esp32s3-elf-gdb"
ELF_PATH = ".pio/build/MIKRO_V2/firmware.elf"

def api_put(endpoint, data):
    url = f"{API_BASE}{endpoint}"
    req = urllib.request.Request(url, data=json.dumps(data).encode(), headers={"Content-Type": "application/json"}, method="PUT")
    try:
        with urllib.request.urlopen(req, timeout=3) as res:
            return json.loads(res.read().decode())
    except Exception as e:
        return {"error": str(e)}

def run_gdb_batch(commands):
    cmd_str = "\n".join(commands) + "\nq\n"
    res = subprocess.run([
        GDB_BIN, "--batch",
        "-ex", "target remote :3333",
        ELF_PATH
    ] + [arg for c in commands for arg in ("-ex", c)], capture_output=True, text=True, timeout=15)
    return res.stdout

def main():
    print("=== USB-JTAG Turn Light Verification ===")
    
    # 1. Start OpenOCD
    print("Starting OpenOCD (builtin JTAG)...")
    openocd_proc = subprocess.Popen(
        ["openocd", "-f", "board/esp32s3-builtin.cfg"],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    time.sleep(2.5)

    try:
        # Check initial state
        print("\n1. Initial JTAG probe: inspecting turnLLed and turnRLed objects in RAM...")
        out = run_gdb_batch([
            "print HardwareInit::turnLPin",
            "print HardwareInit::turnRPin",
            "print HardwareInit::turnLLed._pinMask",
            "print HardwareInit::turnRLed._pinMask",
            "print HardwareInit::turnLLed._channel",
            "print HardwareInit::turnRLed._channel",
            "print HardwareInit::turnLLed._duty",
            "print HardwareInit::turnRLed._duty",
            "print HardwareInit::turnLLed._state",
            "print HardwareInit::turnRLed._state",
            "print VehicleController::s_leftIndActive",
            "print VehicleController::s_rightIndActive",
            "print VehicleController::s_leftIndSuppressed",
            "print VehicleController::s_rightIndSuppressed"
        ])
        print(out)

        # 2. Activate Left Indicator via API
        print("\n2. Activating Left Indicator via Remote API (widget 5 = 1)...")
        api_put("/widgets/5", {"values": [1]})
        time.sleep(0.5)

        out_left = run_gdb_batch([
            "print VehicleController::s_leftIndActive",
            "print VehicleController::s_rightIndActive",
            "print HardwareInit::turnLLed._state",
            "print HardwareInit::turnLLed._duty",
            "print HardwareInit::turnRLed._state",
            "print HardwareInit::turnRLed._duty",
            "x/4xw 0x60019000"  # LEDC register base
        ])
        print(out_left)

        # 3. Activate Right Indicator via API (verifying mutual exclusion under JTAG)
        print("\n3. Activating Right Indicator via Remote API (widget 6 = 1)...")
        api_put("/widgets/6", {"values": [1]})
        time.sleep(0.5)

        out_right = run_gdb_batch([
            "print VehicleController::s_leftIndActive",
            "print VehicleController::s_rightIndActive",
            "print VehicleController::s_leftIndSuppressed",
            "print VehicleController::s_rightIndSuppressed",
            "print HardwareInit::turnLLed._state",
            "print HardwareInit::turnRLed._state"
        ])
        print(out_right)

        # 4. Turn Off Right Indicator via API (widget 6 = 0)
        print("\n4. Turning Off Right Indicator via Remote API (widget 6 = 0)...")
        api_put("/widgets/6", {"values": [0]})
        time.sleep(0.5)

        out_off = run_gdb_batch([
            "print VehicleController::s_leftIndActive",
            "print VehicleController::s_rightIndActive",
            "print HardwareInit::turnLLed._state",
            "print HardwareInit::turnRLed._state",
            "print HardwareInit::turnLLed._duty",
            "print HardwareInit::turnRLed._duty"
        ])
        print(out_off)

    finally:
        openocd_proc.terminate()
        try:
            openocd_proc.wait(timeout=2)
        except Exception:
            openocd_proc.kill()
        print("\n=== JTAG Probe Complete ===")

if __name__ == "__main__":
    main()
