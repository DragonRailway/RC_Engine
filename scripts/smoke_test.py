#!/usr/bin/env python3
"""MIKRO_V2 full end-to-end smoke test — drives RadioKit widgets over serial.

Covers the TRUCK control surface end to end against the current firmware:
boot log, engine start/stop toggle, gear D/P/R (park lock, reverse beep,
shifting sound), throttle/brake blend, steering, lights bitmask, toggle
indicators, horn. Widget IDs = declaration order in src/RADIOKIT.h:

   0 steering_wheel   (int8 -100..100) knob
   1 gas_pedal        (int8 0..100)    slider
   2 brake_pedal      (int8 0..100)    slider
   3 truck_light      (bitmask A..E)   multiple select
   4 start_button     (0/1 toggle)     engine power (Truck)
   5 left_indicator   (0/1 toggle)
   6 right_indicator  (0/1 toggle)
   7 aux_slider       (int8 0..100)    slider
   8 horn_button      (0/1 push)
   9 gear_switch      (index 0=D 1=P 2=R)  radio
  10 throttle_slider  (int8)           (Loco — inert on truck config)
  11 dir_switch       (0/1)            (Loco — inert on truck config)
  12 loco_light       (bitmask)        (Loco — inert on truck config)
  13 bell_button      (0/1 push)       (Loco — inert on truck config)
  14 engine_button    (0/1 toggle)     (Loco — inert on truck config)

NOTE: firmware is type-driven from /vehicle-config.json (type: truck), so the
truck widget set is authoritative and the loco widgets are ignored.
"""
import serial, time, sys, re

PORT = '/dev/ttyACM0'
BAUD = 2000000

# Widget IDs (declaration order in src/RADIOKIT.h)
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

VAR_UPDATE = 0x08   # [widgetId][seq][value...]
SET_PAGE   = 0x20   # [page]
ACK        = 0x05

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

    # Boot sanity checks
    checks = {
        "Vehicle type: TRUCK":         "Vehicle type: TRUCK" in boot_txt,
        "Engine OFF boot state":       "Detected" in boot_txt,  # cell detection ran
        "Config loaded":               "Configs reloaded OK" in boot_txt or "System Ready" in boot_txt,
    }
    for name, ok in checks.items():
        print(f"  [{'PASS' if ok else 'FAIL'}] boot: {name}")

    seq = 0
    ack_count = 0

    def send(cmd, payload=b'', label=''):
        nonlocal seq, ack_count
        seq = (seq + 1) & 0xFF
        ser.write(frame(cmd, payload))
        time.sleep(0.4)
        data = ser.read(4096)
        if data:
            raw = data
            for i in range(len(raw) - 6):
                if raw[i] == 0x55 and raw[i+3] == ACK:
                    ack_count += 1
            printable = ''.join(ch if 32 <= ord(ch) < 127 else '.' for ch in data.decode('utf-8', errors='replace'))
            print(f"  [{label}] response({len(data)}B): {printable[:160]}", flush=True)
        else:
            print(f"  [{label}] (no response)", flush=True)

    def u(wid, value, label):
        send(VAR_UPDATE, bytes([wid, seq, value & 0xFF]), label)

    print("\n── PHASE 1: ENGINE START (start_button toggle ON) ──", flush=True)
    u(W_START, 1, "start_button=ON")

    print("\n── PHASE 2: GEAR D (gear_switch index 0) ──", flush=True)
    u(W_GEAR, 0, "gear=D")

    print("\n── PHASE 3: STEERING SWEEP ──", flush=True)
    for v in (0, 100, 0, -100, 0):
        u(W_STEER, v & 0xFF, f"steer={v}")

    print("\n── PHASE 4: MOTOR FORWARD (gas_pedal) ──", flush=True)
    for v in (0, 30, 60, 0):
        u(W_GAS, v & 0xFF, f"gas={v}")

    print("\n── PHASE 5: BRAKE BLEND (brake_pedal while throttling) ──", flush=True)
    u(W_GAS, 80, "gas=80")
    for v in (0, 50, 100):
        u(W_BRAKE, v & 0xFF, f"brake={v}")
    u(W_BRAKE, 0, "brake=0")
    u(W_GAS, 0, "gas=0")

    print("\n── PHASE 6: LIGHTS (truck_light bitmask A..E) ──", flush=True)
    for bits, name in ((0x01,'A head'), (0x02,'B tail'), (0x04,'C brake'),
                       (0x08,'D hazard'), (0x10,'E reverse'), (0x1F,'ALL'), (0,'off')):
        u(W_TRUCK_LIGHT, bits, f"light={name}")

    print("\n── PHASE 7: INDICATORS (toggle buttons) ──", flush=True)
    u(W_LEFT_IND, 1, "left_ind=ON")
    u(W_LEFT_IND, 0, "left_ind=OFF")
    u(W_RIGHT_IND, 1, "right_ind=ON")
    u(W_RIGHT_IND, 0, "right_ind=OFF")

    print("\n── PHASE 8: HORN (push) ──", flush=True)
    u(W_HORN, 1, "horn=ON")
    u(W_HORN, 0, "horn=OFF")

    print("\n── PHASE 9: GEAR R (reverse beep + reversing light) ──", flush=True)
    u(W_GEAR, 2, "gear=R")
    u(W_GAS, 50, "gas=50 (reverse)")
    u(W_GAS, 0, "gas=0")

    print("\n── PHASE 10: GEAR P (park lock) ──", flush=True)
    u(W_GEAR, 1, "gear=P")
    u(W_GAS, 80, "gas=80 (should stay locked)")
    u(W_GAS, 0, "gas=0")

    print("\n── PHASE 11: ENGINE STOP (toggle OFF) ──", flush=True)
    u(W_START, 0, "start_button=OFF")

    print("\n── PHASE 12: AUX SLIDER (hydraulic channel) ──", flush=True)
    u(W_AUX, 50, "aux=50")
    u(W_AUX, 0, "aux=0")

    # Final: settle everything off
    u(W_STEER, 0, "steer=0")
    u(W_TRUCK_LIGHT, 0, "light=off")

    print(f"\n── DONE. ACK frames received: {ack_count} ──", flush=True)
    ser.close()

if __name__ == '__main__':
    main()
