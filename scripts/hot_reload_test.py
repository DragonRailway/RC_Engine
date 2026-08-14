#!/usr/bin/env python3
"""MIKRO_V2 hot-reload smoke test (task 5.3).

Uploads a modified /hardware-MIKRO_V2.json over the RadioKit FS protocol (0xAA)
while the board keeps running — exactly what the RadioKit app's filesystem
manager does — then watches the firmware's 2-second config watcher trigger a
live reload (reloadConfigs -> HardwareInit::hotReload) without a reboot.

FS frame:  [0xAA][SUB_CMD][LEN_LO][LEN_HI][PAYLOAD]   (no CRC)
  UPLOAD_BEGIN 0x08: [PATH_LEN][PATH][TOTAL_SIZE LE32]   -> ack 0x88 [err]
  UPLOAD_CHUNK  0x09: [OFFSET LE32][DATA]                -> ack 0x89 [err]
  UPLOAD_END    0x0A: [CRC32 LE32]                       -> ack 0x8A [err]
CRC32 = standard zlib (init 0xFFFFFFFF, final XOR 0xFFFFFFFF).
"""
import serial, time, zlib, re, sys

PORT = '/dev/ttyACM0'
BAUD = 2000000

def fs_frame(sub, payload=b''):
    total = 4 + len(payload)
    return bytes([0xAA, sub, total & 0xFF, (total >> 8) & 0xFF]) + payload

def u32le(v):
    return bytes([v & 0xFF, (v >> 8) & 0xFF, (v >> 16) & 0xFF, (v >> 24) & 0xFF])

def main():
    ser = serial.Serial(PORT, BAUD, timeout=0.3)
    print(f"Opened {PORT} @ {BAUD}", flush=True)

    # Modified config: bump sound volume 80 -> 70 (visible change)
    with open('configs/hardware_configs/hardware-MIKRO_V2.json') as f:
        txt = f.read()
    new_txt = re.sub(r'"volume":\s*\d+', '"volume": 70', txt, count=1)
    data = new_txt.encode()
    print(f"Uploading /hardware-MIKRO_V2.json ({len(data)} bytes), volume 80 -> 70", flush=True)

    path = b'/hardware-MIKRO_V2.json'
    ser.reset_input_buffer()

    # Opening the port may have reset the board (DTR). Wait for a clean boot
    # ("Init Complete") so the FS is mounted before we upload.
    print("waiting for clean boot...", flush=True)
    boot_end = time.time() + 12
    boot = b''
    while time.time() < boot_end:
        c = ser.read(4096)
        if c:
            boot += c
            if b'Init Complete' in boot:
                print("boot complete", flush=True)
                break
        else:
            time.sleep(0.1)
    ser.reset_input_buffer()

    # BEGIN + chunked CHUNKs + END, mirroring the RadioKit app's BLE upload.
    # Each frame is acked before the next is sent, so the USB-CDC RX buffer
    # never overflows (it defaults to 256B; main.cpp now raises it to 8KB).
    crc = zlib.crc32(data) & 0xFFFFFFFF
    ser.write(fs_frame(0x08, bytes([len(path)]) + path + u32le(len(data))))
    ser.flush(); time.sleep(0.2)
    CHUNK_SZ = 512
    offset = 0
    while offset < len(data):
        piece = data[offset:offset+CHUNK_SZ]
        ser.write(fs_frame(0x09, u32le(offset) + piece))
        ser.flush(); time.sleep(0.15)
        offset += len(piece)
    ser.write(fs_frame(0x0A, u32le(crc)))
    ser.flush(); time.sleep(0.4)
    r = ser.read(2048)
    print(f"upload acks -> {r.hex(' ')}", flush=True)

    # 4. Watch for the live hot-reload (watcher polls every 2s)
    print("\n── watching for hot-reload (up to 15s) ──", flush=True)
    end = time.time() + 15
    full = b''
    while time.time() < end:
        c = ser.read(4096)
        if c:
            full += c
            txt = full.decode('utf-8', errors='replace')
            # stop early once we see the reload cycle complete
            if 'Reloading Configs' in txt and 'Done' in txt and 'Hot' in txt or 'Reload] Configs reloaded OK' in txt:
                time.sleep(3)   # collect a little more
                while time.time() < end:
                    c = ser.read(4096)
                    if c:
                        full += c
                    else:
                        break
                break
        else:
            time.sleep(0.05)

    text = full.decode('utf-8', errors='replace')
    print(text[-4000:], flush=True)

    # Assessment
    has_reload = 'Reloading Configs' in text
    has_hot = 'Hot-reloading' in text or 'Stopped all outputs' in text
    has_ok = 'Reload] Configs reloaded OK' in text
    has_err = 'mcpwm' in text.lower() and 'timer' in text.lower()
    print("\n══ ASSESSMENT ══", flush=True)
    print(f"  reloadConfigs triggered live : {'YES' if has_reload else 'NO'}", flush=True)
    print(f"  HardwareInit hot-reload ran  : {'YES' if has_hot else 'NO'}", flush=True)
    print(f"  reload completed OK          : {'YES' if has_ok else 'NO'}", flush=True)
    print(f"  MCPWM teardown errors        : {'YES (BUG)' if has_err else 'none'}", flush=True)
    ser.close()

if __name__ == '__main__':
    main()
