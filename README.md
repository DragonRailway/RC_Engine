# 🚗 RC Engine

**RC Engine** is an all-in-one smart controller and dynamic sound synthesizer designed for RC models, scale trucks, crawlers, heavy machinery, tracked vehicles, and model locomotives. Powered by the ESP32-S3 and controlled wirelessly via the **RadioKit** mobile app, RC Engine brings scale models to life with realistic engine physics, authentic acoustics, interactive lighting, and smooth drive control.

---

## ✨ Highlights & Features

### 🔊 Dynamic Physics-Based Sound Engine
* **Real-time Engine Simulation:** Responsive RPM curves, authentic idle rumble, throttle revs, and load-sensitive acceleration.
* **Realistic Mechanical FX:** Turbo spool & blow-off valves, diesel engine knock simulator, supercharger whine, tire squeal, and Jake brake (exhaust engine braking).
* **Work Machine & Scale Details:** Excavator hydraulics, track squeaks, air brake hiss, reversing beepers, air horns, sirens, and train bells.
* **70+ Pre-tuned Vehicle Soundpacks:** Instant access to profiles like Scania V8, Kenworth CAT 3408, Land Rover Defender, Caterpillar excavators & dumpers, Ford Mustang V8, Tatra 8x8, locomotives, and many more!

### 🕹️ Complete Drive & Motion Control
* **Flexible Drivetrains:** Supports standard single-motor ESCs, dual-motor differential drive for tracked tanks and excavators, and precision steering servos.
* **Custom Steering & Throttle Curves:** Adjustable steering sensitivity, endpoint trimming, acceleration ramp rates, and automatic scale braking.
* **Auxiliary Equipment Control:** Dedicated control profiles for dump truck tippers, cement mixer drums, cranes, and winches.

### 💡 Scale Lighting System
* **Automated & Manual Lights:** Headlights (low/high beam), 2-stage tail & brake lights, reversing lights, turn signals, and hazard flashers.
* **Auxiliary & Work Lights:** Cab interior lights, ditch lights, and step lights.
* **Custom LED Animations:** Realistic beacons, strobes, fade effects, and emergency lightbar patterns.

### 📱 Wireless Control & Mobile App (RadioKit)
* **Bluetooth Low Energy (BLE) Connectivity:** Low-latency, zero-lag touch controls directly from your smartphone or tablet.
* **Tailored Cockpits:** Intuitive, switchable UI layouts tailored for road trucks, heavy machinery, and locomotives.
* **Live Battery & Health Telemetry:** Real-time battery voltage monitoring with multi-cell LiPo alerts sent directly to your screen.

### ⚡ On-the-Fly Configuration (Hot-Reload)
* **No Re-flashing Needed:** Easily tune lights, motor endpoints, sound volumes, and physics parameters in human-readable JSON files that update instantly on the board.

---

## 🛠️ Supported Hardware Boards

RC Engine comes ready-to-run on custom ESP32-S3 boards:

| Board | Target Vehicles | Key Features |
|---|---|---|
| **MIKRO V2** | Micro & 1/24–1/35 scale models, custom scale builds | Ultra-compact footprint, dual motor driver, integrated I2S audio amp, multiple LED channels |
| **TRACKLINK V3** | 1/14–1/16 scale trucks, tracked vehicles, crawlers | Dual high-power motor channels, servo outputs, expanded lighting rails, dedicated aux power |

---

## 🚀 Quick Start Guide

### 1. Flash the Board & Vehicle Pack
Prepare your firmware and upload your favorite vehicle sound package to the board's internal storage using the build script:

```bash
# Example: Deploy to a TrackLink V3 with the Scania V8 soundpack
python3 scripts/build_fs.py --board TRACKLINK_V3 --vehicle ScaniaV8
```

### 2. Power On & Connect
1. Connect your battery or power supply to the RC Engine board.
2. Open the **RadioKit** app on your phone / tablet.
3. Select your RC Engine board via Bluetooth (BLE) to open the interactive dashboard.

### 3. Drive & Enjoy!
* **Engine Start/Stop:** Tap the ignition button to start the engine sequence with realistic starter motor cranking and idle settle.
* **Throttle & Steering:** Use responsive on-screen sliders or steering wheels, or pair your favorite physical Bluetooth game controller.
* **Lighting Controls:** Toggle headlights, hazard lights, turn indicators, and beacons with dedicated dashboard buttons.

---

## 🎛️ Customizing Your Vehicle

You can easily adapt RC Engine to match your specific model chassis, gearing, and scale accessories without touching the core firmware code.

* **[Hardware Configuration Guide](GUIDE/HARDWARE_CONFIG.md)** — Learn how to configure motor types, servo directions, lighting pins, and battery voltage thresholds.
* **[Vehicle Configuration Guide](GUIDE/VEHICLE_CONFIG.md)** — Learn how to fine-tune engine physics, gear shifting dynamics, Jake brake strength, and individual audio channel volumes.

---

## 📂 Sound Profile Library Overview

RC Engine includes a rich library of sound configurations under `configs/vehicle_configs/`:

* **Highway & Heavy Trucks:** Scania V8 (1000HP / 143), Kenworth W900A, Peterbilt Detroit Diesel, Mack SuperLiner, Mercedes Actros & SK, Volvo FH16.
* **Offroad & Scalers:** Land Rover Defender (V8 / Td5 / LS3), Toyota Land Cruiser FJ40, Jeep Wrangler Rubicon 392, GMC Sierra, RAM Cummins.
* **Heavy Machinery & Excavators:** Caterpillar 323 / D6, Benford Dumpers, Hitachi ZW370, Volvo L120H / EC550.
* **Military & Utility:** Tatra 813, Ural 375 / 4320, Unimog U1000, GAZ-66, IS-3 Tank, Fire Trucks.
* **Classic & Muscle Cars:** 1965 Ford Mustang V8, Chevy Nova Coupe V8, VW Beetle, Jaguar XJS, LaFerrari.
* **Trains & Aviation:** Union Pacific Diesel Locomotives, Messerschmitt Bf 109.

---

## 📄 License & Credits

* **RCKIT Ecosystem:** RC_Engine is part of the **RCKIT** platform, powered by [RadioKit](https://github.com/) and [ESP32_EasyKit](https://github.com/).
* **Sound Engine & Audio:** Engine sound synthesis concepts and baseline audio assets are adapted from this project [Rc_Engine_Sound_ESP32](https://github.com/TheDIYGuy999/Rc_Engine_Sound_ESP32) by **TheDIYGuy999**.
