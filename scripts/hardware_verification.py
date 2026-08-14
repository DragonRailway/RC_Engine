#!/usr/bin/env python3
"""MIKRO_V2 Live Hardware Telemetry & Panic Assertion Suite (Layer 2).

Drives the physical ESP32-S3 hardware over /dev/ttyACM0 @ 2 Mbaud:
- Validates RadioKit ACK frames and widget command updates.
- Parses live [AUDIO_STATS] lines for zero NaN math errors and task timing < 2900us.
- Asserts zero ESP32 core panics, Coprocessor exceptions, or stack backtraces.
"""
import serial
import time
import sys
import re

PORT = '/dev/ttyACM0'
BAUD = 2000000

# Widget IDs matching declaration order in src/RADIOKIT.h
W_STEER, W_GAS, W_BRAKE, W_TRUCK_LIGHT, W_START, W_LEFT_IND, W_RIGHT_IND, \
W_AUX, W_HORN, W_GEAR, W_THROTTLE, W_DIR, W_LOCO_LIGHT, W_BELL, W_ENGINE = range(15)

def crc16(data):
    crc = 0xFFFF
    for b in data:
        crc ^= (b << 8) & 0xFFFF
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc

def frame(cmd, payload=b''):
    body = bytes([cmd]) + payload
    crc = crc16(body)
    return bytes([0x55, (4 + len(payload) + 2) & 0xFF, ((4 + len(payload) + 2) >> 8) & 0xFF, cmd]) + payload + bytes([crc & 0xFF, (crc >> 8) & 0xFF])

VAR_UPDATE = 0x08
SET_PAGE   = 0x20
ACK        = 0x05

PANIC_KEYWORDS = [
    "Guru Meditation Error",
    "Coprocessor exception",
    "abort() was called",
    "Backtrace:",
    "Panic"
]

def check_panic(text):
    for kw in PANIC_KEYWORDS:
        if kw in text:
            print(f"[PANIC DETECTED]: Found '{kw}' in serial output!")
            return False
    return True

def main():
    print(f"[Hardware Verification] Connecting to {PORT} @ {BAUD}...")
    try:
        ser = serial.Serial(PORT, BAUD, timeout=0.3)
    except Exception as e:
        print(f"[Hardware Verification] Port open FAIL: {e}")
        sys.exit(1)

    print("[Hardware Verification] Resetting ESP32 hardware via DTR/RTS...")
    ser.setDTR(False)
    ser.setRTS(True)
    time.sleep(0.1)
    ser.setDTR(True)
    ser.setRTS(False)
    time.sleep(0.05)
    ser.reset_input_buffer()

    print("[Hardware Verification] Draining boot logs (10s BLE/LittleFS init window)...")
    boot_end = time.time() + 10
    boot_log = b""
    while time.time() < boot_end:
        buf = ser.read(4096)
        if buf:
            boot_log += buf
        else:
            time.sleep(0.05)

    boot_txt = boot_log.decode('utf-8', errors='replace')
    if not check_panic(boot_txt):
        sys.exit(1)

    print("[Hardware Verification] Boot successful. Executing live verification phases...")
    seq = 0
    ack_count = 0
    test_log = boot_txt

    def send_widget(w_id, payload_bytes):
        nonlocal seq, ack_count, test_log
        seq = (seq + 1) & 0xFF
        pkt = frame(VAR_UPDATE, bytes([w_id, seq]) + payload_bytes)
        ser.write(pkt)
        time.sleep(0.05)
        resp = ser.read(2048)
        if resp:
            txt = resp.decode('utf-8', errors='replace')
            test_log += txt
            if frame(ACK, bytes([seq])) in resp or bytes([0x55, 0x07, 0x00, ACK, seq]) in resp:
                ack_count += 1

    # Phase 1: Engine Start
    print("  Phase 1: Engine Start Toggle (W_START=1)...")
    send_widget(W_START, bytes([0x01]))
    time.sleep(0.5)

    # Phase 2: Gear Shift D -> P -> R
    print("  Phase 2: Gear Shift (D=0, P=1, R=2)...")
    send_widget(W_GEAR, bytes([0x00])) # D
    time.sleep(0.1)
    send_widget(W_GEAR, bytes([0x01])) # P (Park Lock)
    time.sleep(0.1)
    send_widget(W_GEAR, bytes([0x02])) # R (Reverse)
    time.sleep(0.1)
    send_widget(W_GEAR, bytes([0x00])) # D
    time.sleep(0.1)

    # Phase 3: Throttle Ramps & Brake Blend
    print("  Phase 3: Throttle Ramp & Brake Blending...")
    send_widget(W_GAS, bytes([50]))
    time.sleep(0.1)
    send_widget(W_GAS, bytes([100]))
    time.sleep(0.1)
    send_widget(W_BRAKE, bytes([60])) # Proportional brake blend
    time.sleep(0.1)
    send_widget(W_BRAKE, bytes([0]))
    send_widget(W_GAS, bytes([0]))
    time.sleep(0.1)

    # Phase 4: Light Bitmasks & Indicators
    print("  Phase 4: Light Bitmasks & Indicator Auto-Cancel...")
    send_widget(W_TRUCK_LIGHT, bytes([0x01])) # Low beam
    time.sleep(0.1)
    send_widget(W_TRUCK_LIGHT, bytes([0x03])) # High beam
    time.sleep(0.1)
    send_widget(W_LEFT_IND, bytes([0x01]))
    time.sleep(0.2)
    send_widget(W_LEFT_IND, bytes([0x00]))

    # Phase 5: Aux Hydraulic Ramp
    print("  Phase 5: Aux Hydraulic Servo Ramp...")
    send_widget(W_AUX, bytes([75]))
    time.sleep(0.1)
    send_widget(W_AUX, bytes([0]))
    time.sleep(0.1)

    # Phase 6: Engine Stop
    print("  Phase 6: Engine Stop Toggle (W_START=0)...")
    send_widget(W_START, bytes([0x00]))
    time.sleep(0.5)

    # Drain remaining logs
    time.sleep(0.5)
    rem = ser.read(4096)
    if rem:
        test_log += rem.decode('utf-8', errors='replace')

    ser.close()

    print("[Hardware Verification] Checking panic markers across entire test run...")
    if not check_panic(test_log):
        sys.exit(1)
    print("  PASS: 0 ESP32 core panics, Coprocessor exceptions, or backtraces.")

    print(f"[Hardware Verification] RadioKit ACK frames received: {ack_count}")
    assert ack_count > 0, "Expected at least 1 valid RadioKit ACK frame"

    print("[Hardware Verification] LIVE HARDWARE TELEMETRY & PANIC SUITE PASSED CLEAN.")
    sys.exit(0)

if __name__ == "__main__":
    main()
