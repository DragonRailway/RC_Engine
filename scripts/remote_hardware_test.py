import urllib.request
import json
import time
import serial
import threading

BASE_URL = "http://127.0.0.1:17007/api"

def api_put(endpoint, data):
    url = f"{BASE_URL}{endpoint}"
    req = urllib.request.Request(url, data=json.dumps(data).encode(), headers={"Content-Type": "application/json"}, method="PUT")
    try:
        with urllib.request.urlopen(req, timeout=3) as res:
            return json.loads(res.read().decode())
    except Exception as e:
        return {"error": str(e)}

def api_post(endpoint, data):
    url = f"{BASE_URL}{endpoint}"
    req = urllib.request.Request(url, data=json.dumps(data).encode(), headers={"Content-Type": "application/json"}, method="POST")
    try:
        with urllib.request.urlopen(req, timeout=3) as res:
            return json.loads(res.read().decode())
    except Exception as e:
        return {"error": str(e)}

def api_get(endpoint):
    url = f"{BASE_URL}{endpoint}"
    req = urllib.request.Request(url)
    try:
        with urllib.request.urlopen(req, timeout=3) as res:
            return json.loads(res.read().decode())
    except Exception as e:
        return {"error": str(e)}

print("=== Remote Hardware Diagnostic Test ===")

# Check connection
conn = api_get("/connection")
print("Initial Connection Status:", conn.get("connected"), "Device:", conn.get("device", {}).get("name") if conn.get("device") else None)
if not conn.get("connected"):
    print("Scanning for BLE devices...")
    api_post("/pair/scan", {"type": "ble"})
    time.sleep(3)
    devs = api_get("/pair/devices")
    print("Discovered Devices:", devs)
    target_id = None
    if "devices" in devs and len(devs["devices"]) > 0:
        for d in devs["devices"]:
            if "Scania" in d.get("name", "") or "RC" in d.get("name", ""):
                target_id = d.get("id")
                break
        if not target_id:
            target_id = devs["devices"][0].get("id")
    
    if target_id:
        print(f"Connecting to BLE device: {target_id}...")
        api_post("/connection/connect", {"id": target_id, "type": "ble"})
        time.sleep(3)
    else:
        print("Attempting reconnect to last device...")
        api_post("/connection/reconnect", {})
        time.sleep(3)

conn = api_get("/connection")
print("Post-Connect Status:", conn.get("connected"))

# Open serial logger thread to capture firmware [STATUS] and [EVENT] lines
serial_lines = []
stop_serial = False

def serial_reader():
    try:
        ser = serial.Serial("/dev/ttyACM0", 2000000, timeout=0.1)
        while not stop_serial:
            line = ser.readline().decode("utf-8", errors="ignore")
            if line:
                line_str = line.strip()
                if "[STATUS]" in line_str or "[EVENT]" in line_str or "[Lights]" in line_str or "[HardwareInit]" in line_str:
                    serial_lines.append(line_str)
                    print("  [FW LOG]", line_str)
        ser.close()
    except Exception as e:
        print("Serial reader exception:", e)

th = threading.Thread(target=serial_reader, daemon=True)
th.start()

time.sleep(1)

# Step 1: Zero out pedals and controls (idle = -100 for RK_GasPedal / RK_SPRING_MIN)
print("\n--- Step 1: Initializing Safe Controls ---")
api_put("/widgets/2", {"values": [-100]})  # brake_pedal = -100 (0% brake)
api_put("/widgets/1", {"values": [-100]})  # gas_pedal = -100 (0% throttle)
api_put("/widgets/0", {"values": [0]})     # steering_wheel = 0 (center)
api_put("/widgets/3", {"values": [0]})     # truck_light = 0 (off)
api_put("/widgets/5", {"values": [0]})     # left_indicator = 0 (off)
api_put("/widgets/6", {"values": [0]})     # right_indicator = 0 (off)
api_put("/widgets/9", {"values": [1]})     # gear = P (1)
time.sleep(1)

# Step 2: Start Engine
print("\n--- Step 2: Starting Engine (start_button ON) ---")
api_put("/widgets/4", {"values": [1]})     # start_button = 1
time.sleep(3)

# Step 3: Shift to Drive (D)
print("\n--- Step 3: Shifting Gear to Drive (D=0) ---")
api_put("/widgets/9", {"values": [0]})     # gear_switch = 0 (D)
time.sleep(1)

# Step 4: Apply Throttle (50% throttle -> gas_pedal = 0; 100% throttle -> gas_pedal = 100)
print("\n--- Step 4: Applying 50% Throttle (gas_pedal = 0) ---")
api_put("/widgets/1", {"values": [0]})      # (0 + 100)/2 = 50% throttle
time.sleep(1.5)
print("  Applying 100% Throttle (gas_pedal = 100)...")
api_put("/widgets/1", {"values": [100]})    # (100 + 100)/2 = 100% throttle
time.sleep(1.5)
print("  Releasing Throttle (gas_pedal = -100)...")
api_put("/widgets/1", {"values": [-100]})   # 0% throttle
time.sleep(1)

# Step 5: Steering Sweep (Left -> Center -> Right)
print("\n--- Step 5: Testing Steering Servo (-50 -> +50 -> 0) ---")
api_put("/widgets/0", {"values": [-50]})
time.sleep(1)
api_put("/widgets/0", {"values": [50]})
time.sleep(1)
api_put("/widgets/0", {"values": [0]})
time.sleep(1)

# Step 6: Test Brake Pedal
print("\n--- Step 6: Testing Brake Pedal (brake_pedal = 20 -> 60% brake) ---")
api_put("/widgets/2", {"values": [20]})     # (20 + 100)/2 = 60% brake
time.sleep(1.5)
api_put("/widgets/2", {"values": [-100]})   # released
time.sleep(1)

# Step 7: Turn Signal Edge Latching & Cancellation Test
print("\n--- Step 7: Testing Turn Signal Toggle & Steering Auto-Cancellation ---")
print("  Activating Left Indicator...")
api_put("/widgets/5", {"values": [1]})
time.sleep(1)
print("  Steering Left -40% (Arming)...")
api_put("/widgets/0", {"values": [-40]})
time.sleep(1)
print("  Returning Steering Wheel to 0 (Auto-Cancelling)...")
api_put("/widgets/0", {"values": [0]})
time.sleep(1.5)

# Step 8: Test Lighting Grid Modes
print("\n--- Step 8: Testing Lights (Headlights -> High Beam -> Fog -> Hazard -> Beacon) ---")
print("  Headlights (Low Beam 40%)...")
api_put("/widgets/3", {"values": [1]})   # Bit 0 = Headlights
time.sleep(1.5)

print("  High Beam (100%)...")
api_put("/widgets/3", {"values": [3]})   # Bit 0 + 1 = High Beam
time.sleep(1.5)

print("  Fog Lamp...")
api_put("/widgets/3", {"values": [7]})   # Bit 0 + 1 + 2 = Fog
time.sleep(1.5)

print("  Hazard Lights (Blink)...")
api_put("/widgets/3", {"values": [15]})  # Bit 0..3 = Hazard
time.sleep(2)

print("  Beacon Strobe...")
api_put("/widgets/3", {"values": [31]})  # Bit 0..4 = Beacon
time.sleep(2)

print("  Lights OFF...")
api_put("/widgets/3", {"values": [0]})
time.sleep(1)

# Step 9: Auxiliary Motor / Hydraulics
print("\n--- Step 9: Testing Aux Motor Slider (aux_slider = 50) ---")
api_put("/widgets/7", {"values": [50]})
time.sleep(1.5)
api_put("/widgets/7", {"values": [0]})
time.sleep(1)

# Step 10: Horn
print("\n--- Step 10: Testing Horn Button ---")
api_put("/widgets/8", {"values": [1]})
time.sleep(0.5)
api_put("/widgets/8", {"values": [0]})
time.sleep(1)

# Step 11: Stop Throttle & Shift to Park
print("\n--- Step 11: Parking and Stopping Engine ---")
api_put("/widgets/1", {"values": [-100]})
api_put("/widgets/9", {"values": [1]})   # Park (P)
api_put("/widgets/4", {"values": [0]})   # start_button = 0
time.sleep(2)

stop_serial = True
th.join(timeout=1)
print("\n=== Remote Hardware Test Complete ===")
