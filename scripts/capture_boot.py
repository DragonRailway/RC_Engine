#!/usr/bin/env python3
"""Capture the boot log and print the sound-loading section."""
import serial, time

ser = serial.Serial('/dev/ttyACM0', 2000000, timeout=0.3)
ser.setDTR(False); ser.setRTS(True); time.sleep(0.1)
ser.setDTR(True); ser.setRTS(False); time.sleep(0.05)
ser.reset_input_buffer()

t_end = time.time() + 10
boot = b''
while time.time() < t_end:
    c = ser.read(4096)
    if c:
        boot += c
    else:
        time.sleep(0.05)
txt = boot.decode('utf-8', errors='replace')
i = txt.find('Loading Sounds')
if i >= 0:
    print(txt[i-200:i+1400])
else:
    print(txt[-2500:])
