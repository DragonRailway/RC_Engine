#!/usr/bin/env python3
"""MIKRO_V2 hardware smoke test — drives RadioKit widgets over serial.

Drives the truck via the RadioKit serial transport (0x55 protocol) on the
hardware USB serial. Widget IDs (declaration order in src/RADIOKIT.h):
  0 steering_wheel (int8 -100..100)   -> servo
  1 gas_pedal      (int8)             -> motor throttle (page 0)
  2 brake_pedal    (int8)
  3 led_select     (bitmask)          -> lights (A..E) (page 0)
  4 slider         (int8)             -> motor throttle (page 1)
  5 dir_switch     (0/1)              -> reverse (page 1)
  6 lights_toggle  (bitmask)          -> lights (page 1)
  7 horn           (0/1)              -> horn (page 1)
"""
import serial, time, sys

PORT = '/dev/ttyACM0'
BAUD = 2000000

def crc16(data):
    crc = 0xFFFF
    for b in data:
        crc ^= (b << 8) & 0xFFFF
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc

def frame(cmd, payload=b''):
    body = bytes([cmd]) + payload
    total = 4 + len(payload) + 2
    crc = crc16(body)
    return bytes([0x55, total & 0xFF, (total >> 8) & 0xFF, cmd]) + payload + bytes([crc & 0xFF, (crc >> 8) & 0xFF])

VAR_UPDATE = 0x08   # [widgetId][seq][value...]
SET_PAGE   = 0x20   # [page]
ACK        = 0x05
PAGE_CHANGED = 0x21

def main():
    ser = serial.Serial(PORT, BAUD, timeout=0.3)
    print(f"Opened {PORT} @ {BAUD}", flush=True)

    # Reset the board for a clean boot
    ser.setDTR(False); ser.setRTS(True); time.sleep(0.1)
    ser.setDTR(True); ser.setRTS(False); time.sleep(0.05)
    ser.reset_input_buffer()

    # Drain boot log
    boot_end = time.time() + 8
    boot = b''
    while time.time() < boot_end:
        c = ser.read(4096)
        if c: boot += c
        else: time.sleep(0.05)
    boot_txt = boot.decode('utf-8', errors='replace')
    print("── boot log (tail) ──")
    print(boot_txt[-1500:])
    ser.reset_input_buffer()

    seq = 0
    ack_count = 0

    def send(cmd, payload=b'', label=''):
        nonlocal seq, ack_count
        seq = (seq + 1) & 0xFF
        ser.write(frame(cmd, payload))
        time.sleep(0.4)
        # Drain + look for ACK
        data = ser.read(4096)
        if data:
            text = data.decode('utf-8', errors='replace')
            if cmd == VAR_UPDATE and b'\x55' in data:
                # find ACK frames (0x55 09 00 05 seq crc crc)
                raw = data
                for i in range(len(raw) - 6):
                    if raw[i] == 0x55 and raw[i+3] == ACK:
                        ack_count += 1
            printable = ''.join(ch if 32 <= ord(ch) < 127 else '.' for ch in text)
            print(f"  [{label}] response({len(data)}B): {printable[:200]}", flush=True)
        else:
            print(f"  [{label}] (no response)", flush=True)

    print("\n── PHASE 1: SERVO SWEEP (steering_wheel) ──", flush=True)
    for v in (0, 100, 0, -100, 0):
        send(VAR_UPDATE, bytes([0, seq, (v & 0xFF)]), f"steer={v}")

    print("\n── PHASE 2: LIGHTS (led_select bitmask) ──", flush=True)
    for bits, name in ((0x01,'A head'), (0x02,'B tail'), (0x04,'C brake'),
                       (0x08,'D turn'), (0x10,'E reverse'), (0x1F,'ALL'), (0,'off')):
        send(VAR_UPDATE, bytes([3, seq, bits]), f"led={name}")

    print("\n── PHASE 3: MOTOR FORWARD (gas_pedal) ──", flush=True)
    for v in (0, 30, 60, 0):
        send(VAR_UPDATE, bytes([1, seq, (v & 0xFF)]), f"gas={v}")

    print("\n── PHASE 4: REVERSE (switch to Loco page) ──", flush=True)
    send(SET_PAGE, bytes([1]), "page=1")
    send(VAR_UPDATE, bytes([5, seq, 1]), "dir_switch=reverse")
    send(VAR_UPDATE, bytes([4, seq, 40]), "slider=40")
    send(VAR_UPDATE, bytes([4, seq, 0]), "slider=0")
    send(VAR_UPDATE, bytes([5, seq, 0]), "dir_switch=fwd")
    send(SET_PAGE, bytes([0]), "page=0")

    print("\n── PHASE 5: HORN (Loco page) ──", flush=True)
    send(SET_PAGE, bytes([1]), "page=1")
    send(VAR_UPDATE, bytes([7, seq, 1]), "horn=on")
    send(VAR_UPDATE, bytes([7, seq, 0]), "horn=off")
    send(SET_PAGE, bytes([0]), "page=0")

    # Final: settle everything off
    send(VAR_UPDATE, bytes([1, seq, 0]), "gas=0")
    send(VAR_UPDATE, bytes([0, seq, 0]), "steer=0")
    send(VAR_UPDATE, bytes([3, seq, 0]), "led=off")

    print(f"\n── DONE. ACK frames received: {ack_count} ──", flush=True)
    ser.close()

if __name__ == '__main__':
    main()
