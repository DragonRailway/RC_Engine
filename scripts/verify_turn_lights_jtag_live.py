#!/usr/bin/env python3
"""
USB-JTAG Live Hardware Register & Pin Verification for Turn Lights
Connects via OpenOCD JTAG, samples hardware registers (GPIO_OUT1_REG, LEDC_CHx_DUTY_REG)
while driving turn lights over the RadioKit Remote API.
"""

import subprocess
import time
import socket
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

class OpenOcdTelnet:
    def __init__(self, host="127.0.0.1", port=4444):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((host, port))
        time.sleep(0.5)
        self._read_until_prompt()

    def _read_until_prompt(self):
        buf = b""
        while b"> " not in buf:
            chunk = self.sock.recv(1024)
            if not chunk:
                break
            buf += chunk
        return buf.decode("utf-8", errors="ignore")

    def cmd(self, command):
        self.sock.sendall((command + "\n").encode("utf-8"))
        res = self._read_until_prompt()
        return res

    def halt(self):
        return self.cmd("halt")

    def resume(self):
        return self.cmd("resume")

    def read_mem32(self, addr, count=1):
        raw = self.cmd(f"mdw {hex(addr)} {count}")
        # Parses "0x6000400c: 00000000 ..."
        words = []
        for line in raw.splitlines():
            line = line.strip()
            if line.startswith("0x"):
                parts = line.split(":")
                if len(parts) == 2:
                    vals = parts[1].strip().split()
                    for v in vals:
                        try:
                            words.append(int(v, 16))
                        except ValueError:
                            pass
        return words

    def close(self):
        self.sock.close()

def main():
    print("=== USB-JTAG Live Turn Light Hardware Probe ===")
    
    jtag = OpenOcdTelnet()
    print("Connected to OpenOCD JTAG Telnet server.")

    try:
        # Ensure target is running
        print("Resuming ESP32-S3 target execution...")
        jtag.resume()
        time.sleep(2)

        # Baseline: Indicators OFF
        print("\n--- Test Baseline: Indicators OFF ---")
        api_put("/widgets/5", {"values": [0]})
        api_put("/widgets/6", {"values": [0]})
        time.sleep(1)

        jtag.halt()
        gpio_out1 = jtag.read_mem32(0x6000400C, 1)[0]
        gpio_en1 = jtag.read_mem32(0x60004028, 1)[0]
        ledc5_duty = jtag.read_mem32(0x60019078, 1)[0]
        ledc6_duty = jtag.read_mem32(0x6001908C, 1)[0]
        jtag.resume()

        p41_en = bool(gpio_en1 & (1 << 9))
        p42_en = bool(gpio_en1 & (1 << 10))
        print(f"GPIO 41 (Turn L) Output Enabled: {p41_en}, LEDC Ch5 Duty: {ledc5_duty}")
        print(f"GPIO 42 (Turn R) Output Enabled: {p42_en}, LEDC Ch6 Duty: {ledc6_duty}")

        # Activate Left Turn Signal
        print("\n--- Activating Left Indicator (widget 5 = 1) ---")
        api_put("/widgets/5", {"values": [1]})
        
        # Sample blinking over 2 seconds
        print("Sampling LEDC Ch5 (GPIO 41) duty across blink cycle...")
        samples_l = []
        for i in range(10):
            time.sleep(0.15)
            jtag.halt()
            d5 = jtag.read_mem32(0x60019078, 1)[0]
            d6 = jtag.read_mem32(0x6001908C, 1)[0]
            jtag.resume()
            samples_l.append((d5, d6))
            print(f"  Sample {i+1}: Ch5(Left)={d5} (0x{d5:X}), Ch6(Right)={d6}")

        # Activate Right Turn Signal (Mutual Exclusion)
        print("\n--- Activating Right Indicator (widget 6 = 1, Mutual Exclusion) ---")
        api_put("/widgets/6", {"values": [1]})
        
        # Sample blinking over 2 seconds
        print("Sampling LEDC Ch6 (GPIO 42) duty across blink cycle...")
        samples_r = []
        for i in range(10):
            time.sleep(0.15)
            jtag.halt()
            d5 = jtag.read_mem32(0x60019078, 1)[0]
            d6 = jtag.read_mem32(0x6001908C, 1)[0]
            jtag.resume()
            samples_r.append((d5, d6))
            print(f"  Sample {i+1}: Ch5(Left)={d5}, Ch6(Right)={d6} (0x{d6:X})")

        # Turn Signals OFF
        print("\n--- Turning Indicators OFF (widget 6 = 0) ---")
        api_put("/widgets/6", {"values": [0]})
        time.sleep(1)

        jtag.halt()
        d5_off = jtag.read_mem32(0x60019078, 1)[0]
        d6_off = jtag.read_mem32(0x6001908C, 1)[0]
        jtag.resume()
        print(f"Post-turnoff LEDC Ch5={d5_off}, Ch6={d6_off}")

    finally:
        try:
            jtag.resume()
            jtag.close()
        except Exception:
            pass
        print("\n=== Turn Light JTAG Verification Complete ===")

if __name__ == "__main__":
    main()
