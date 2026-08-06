#!/usr/bin/env python3
"""Capture serial output from the board for N seconds into a log file."""
import serial, sys, time

PORT = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyACM0"
OUT = sys.argv[2] if len(sys.argv) > 2 else "/tmp/serial_capture.log"
SECS = float(sys.argv[3]) if len(sys.argv) > 3 else 10.0

ser = serial.Serial(PORT, 2000000, timeout=0.3)
ser.setDTR(False)
ser.setRTS(False)
ser.reset_input_buffer()

end = time.time() + SECS
full = b""
while time.time() < end:
    c = ser.read(4096)
    if c:
        full += c
    else:
        time.sleep(0.05)
ser.close()

with open(OUT, "wb") as f:
    f.write(full)
print(f"captured {len(full)} bytes -> {OUT}")
