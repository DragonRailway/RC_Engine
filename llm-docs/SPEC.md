# RC Brain - Technical Specification

## 1. Project Overview

RC Brain is an ESP32-S3 based RC vehicle sound and control controller designed for the TRACKLINK_V3 board (Elecrow). It provides realistic engine sound simulation, motor control, servo control, and lighting management for RC vehicles.

### 1.1 Key Features
- Multi-layered engine sound synthesis (idle, rev, knock, turbo, wastegate)
- Automatic/manual transmission simulation
- H-Bridge DC motor control with BEMF sensing
- Servo control for steering and gearbox
- Multi-channel LED lighting with effects (blink, fade, sequential)
- I2S audio output (22,050 Hz, 8-bit PCM)
- JSON-based configuration via LittleFS
- 80+ pre-built vehicle sound profiles

---

## 2. Hardware Specifications

### 2.1 TRACKLINK_V3 Board
- **MCU**: ESP32-S3 N4R2
- **Flash**: 4MB, QIO mode, 80 MHz
- **PSRAM**: 2MB, QSPI (Quad SPI)
- **Memory Type**: QIO QSPI

### 2.2 Pin Mapping

| Function | Pin | Notes |
|----------|-----|-------|
| **H-Bridge A** | PWM1=13, PWM2=14, BEMF=4 | Motor control |
| **H-Bridge B** | DIR=15, PWM=16, BEMF=5 | Motor control |
| COMMON_EN | 12 | H-Bridge enable |
| **LED Outputs** | L0=42, L1-L6=6-11 | Light channels |
| **Servo Outputs** | S1=1, S2=2 | Steering/Gearbox |
| **I2S Audio** | LRC=17, BCLK=18, DIN=21, SD=47 | Audio output |
| **Power** | ENABLE=33, BUTTON=48, V_ADC=2, CHRG_SENS=34 | Power management |

### 2.3 PWM Configuration
- **LED PWM**: 8 kHz, 10-bit resolution (0-1023)
- **Motor PWM**: 24 kHz, 8-bit resolution (0-255)
- **Servo PWM**: 50 Hz, 1000-2000 μs pulse width

---

## 3. Detailed Documentation

| Document | Description |
|----------|-------------|
| [CONFIGURATION.md](CONFIGURATION.md) | JSON configuration files, file system, config loading |
| [SIMULATION.md](SIMULATION.md) | Sound engine, RPM simulation, engine/transmission behavior |
| [MOTORS.md](MOTORS.md) | H-Bridge, ESC, servo, gearbox control |
| [LIGHTS.md](LIGHTS.md) | LED channels, effects, light types |
| [BOARD.md](BOARD.md) | Board definitions, pin mapping, creating new boards |

---

## 4. Build System

### 4.1 PlatformIO Configuration
- **Platform**: espressif32 (pioarduino)
- **Framework**: Arduino
- **Board**: esp32-s3-devkitc-1
- **Flash**: 4MB
- **PSRAM**: 2MB QSPI
- **Partition Table**: huge_app.csv

### 4.2 Dependencies
| Library | Version | Purpose |
|---------|---------|---------|
| ArduinoJson | ^7.0.4 | JSON parsing |
| ESP32_PWM_Fusion | local | PWM control |
| RcEngineSound | local | Sound engine |
| LittleFS | built-in | File system |
| I2S | built-in | Audio output |

### 4.3 Build Commands
```bash
pio run                    # Build project
pio run -e TRACKLINK_V3    # Build specific environment
pio run -t clean           # Clean build
pio run -t upload          # Upload to device
pio device monitor         # Serial monitor (2000000 baud)
pio test                   # Run unit tests
```

---

## 5. Vehicle Profiles

### 5.1 Supported Vehicle Types
| Type | Description | Special Features |
|------|-------------|------------------|
| TRUCK | Semi-trucks, pickups | Jake brake, air brake, gear shifting |
| CAR | Cars, SUVs | Rev matching, tire squeal |
| TANK | Tracked vehicles | Dual throttle, track rattle |
| EXCAVATOR | Construction equipment | Hydraulic sounds, bucket rattle |
| LOCOMOTIVE | Train engines | Bell, whistle, super slow accel |

### 5.2 Pre-built Profiles (80+)
Located in `lib/RcEngineSound/vehicles/`:
- Scania V8, Scania 143
- Kenworth W900, Peterbilt
- Mercedes Actros, MAN TGX
- Volvo FH16
- Land Rover Defender V8
- Caterpillar 323 Excavator
- Tatra 813
- And many more...

---

## 6. Configuration Architecture

### 6.1 Separation of Concerns
- **Hardware Config** (`hardware-*.json`): User-defined, board-specific pin mappings and peripheral settings
- **Vehicle Config** (`vehicle-*.json`): Hardware-agnostic, portable vehicle behavior and sound parameters

This allows vehicle profiles to be shared across different hardware setups without modification.

### 6.2 File System Structure

```
/
├── hardware-*.json          # Hardware configuration (user-specific)
├── vehicle-*.json           # Vehicle configuration (portable)
└── sounds/
    ├── {Vehicle}-idle.json  # Idle sound samples
    ├── {Vehicle}-rev.json   # Rev sound samples
    ├── {Vehicle}-start.json # Start sound samples
    ├── {Vehicle}-knock.json # Diesel knock samples
    ├── {Vehicle}-horn.json  # Horn sounds
    └── ...
```

---

## 7. Serial Debug Output

- **Default**: 2,000,000 baud
- **USB CDC**: Enabled on boot
- **Debug Level**: 5 (Verbose)

---

## 8. Future Considerations

### 8.1 Not Yet Implemented
- DSHOT motor protocol
- Bluetooth configuration
- OTA firmware updates
- Multi-board networking
- Advanced hydraulic simulation

### 8.2 Potential Enhancements
- WiFi configuration portal
- Mobile app for sound selection
- Real-time sound mixing adjustments
- Telemetry data logging
- Advanced lighting animations