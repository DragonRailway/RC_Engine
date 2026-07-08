# Configuration System

## Overview

The RC Brain uses two separate JSON configuration files:

1. **Hardware Config** (`hardware-*.json`) - User-defined, board-specific
2. **Vehicle Config** (`vehicle-*.json`) - Hardware-agnostic, portable across any board

This separation allows vehicle profiles to be shared across different hardware setups.

## 1. File System

- **Storage**: LittleFS (SPIFFS successor)
- **Mount Point**: `/`
- **Config Location**: Root directory
- **Sound Location**: `/sounds/`

## 2. Hardware Config (`hardware-*.json`)

**User-configured for their specific board and wiring connections.**

Defines physical hardware connections: which pins drive motors, servos, and LEDs. This file is unique to each user's build.

```json
{
  "SOUND": {
    "VOLUME": 80                    // Master volume (0-100%)
  },
  "DRIVE_MOTOR": {
    "HARDWARE": "HBRIDGE_A",       // HBRIDGE_A, HBRIDGE_B, S1, S2
    "FREQUENCY": 20000,            // PWM frequency (Hz)
    "DIRECTION": "FORWARD",        // FORWARD, REVERSE, UNI_FORWARD, UNI_REVERSE
    "DUTY": {
      "MIN": 20,                   // Minimum duty cycle (%)
      "MAX": 90                    // Maximum duty cycle (%)
    }
  },
  "STEERING_SERVO": {
    "HARDWARE": "S1",              // Servo pin
    "FREQUENCY": 50,               // PWM frequency (Hz)
    "ENDPOINTS": {
      "LEFT": 1350,                // Pulse width (μs)
      "RIGHT": 1650,
      "CENTER": 1500
    }
  },
  "LIGHTS": {
    "HEAD_LIGHT": {
      "HARDWARE": "L1",            // Pin name from board definition
      "BRIGHTNESS_MAX": 60         // Brightness (0-100%)
    },
    "TAIL_LIGHT": {
      "HARDWARE": "L2",
      "BRIGHTNESS_MAX": 60
    },
    "BRAKE_LIGHT": {
      "HARDWARE": "L3"
    },
    "TURN_LIGHT": {
      "LEFT": { "HARDWARE": "L4" },
      "RIGHT": { "HARDWARE": "L5" },
      "BRIGHTNESS_MAX": 60,
      "TYPE": "BLINK",             // BLINK, FADE, SEQUENTIAL
      "INTERVAL_ON": 500,          // On duration (ms)
      "INTERVAL_OFF": 500          // Off duration (ms)
    },
    "REVERSING_LIGHT": {
      "HARDWARE": "BRAKE_LIGHT"    // References another light
    }
  }
}
```

### Pin Name Resolution

Hardware config references pins by name. The `PinMapper` class resolves names to GPIOs:

| Name | GPIO | Function |
|------|------|----------|
| L0 | 42 | Built-in LED |
| L1-L6 | 6-11 | General LEDs |
| S1 | 1 | Servo/ESC 1 |
| S2 | 2 | Servo/ESC 2 |
| HBRIDGE_A | {13,14,4} | H-Bridge A (PWM1, PWM2, BEMF) |
| HBRIDGE_B | {16,15,5} | H-Bridge B (PWM, DIR, BEMF) |

## 3. Vehicle Config (`vehicle-*.json`)

**Hardware-agnostic - works on any board with any wiring.**

Defines vehicle behavior: engine parameters, transmission, and sound volumes. These settings are purely logical and have no dependency on specific hardware pins.

A single vehicle config can be used on a truck with H-Bridge motors, an ESC-based car, or a servo-only setup - the hardware layer handles the differences.

```json
{
  "VEHICLE": {
    "NAME": "Scania V8",
    "TYPE": "TRUCK"                // TRUCK, CAR, TANK, EXCAVATOR, LOCOMOTIVE
  },
  "ENGINE": {
    "ACCELERATION": 2,             // 1=slowest/heavy, 9=fastest/light
    "DECELERATION": 1,             // 1=slowest/heavy, 5=fastest/light
    "IDLE_RPM": 10,                // Idle engine speed
    "CLUTCH_RPM": 100,             // Clutch engagement point
    "REV_SWITCH_POINT": 50,        // Rev sound mix start (%)
    "IDLE_END_POINT": 40,          // Idle sound end (%)
    "DIESEL_KNOCK_INTERVAL": 8,    // Pulses per cycle (e.g., 8 for V8)
    "DIESEL_KNOCK_START_POINT": 30,// Knock volume start (%)
    "JAKEBRAKE_MIN_RPM": 60,       // Jake brake min RPM
    "FAN_START_POINT": 0           // Cooling fan start (%)
  },
  "TRANSMISSION": {
    "TYPE": "AUTOMATIC",           // AUTOMATIC, MANUAL, NONE
    "NUMBER_OF_GEARS": 3           // 3, 4, or 6
  },
  "SOUND_VOLUME": {
    "START": 140,                  // Engine start volume (%)
    "IDLE": 80,                    // Idle volume (%)
    "ENGINE_IDLE": 50,             // Base engine block volume (%)
    "FULL_THROTTLE": 150,          // Max throttle volume (%)
    "REV": 100,                    // Rev volume (%)
    "ENGINE_REV": 50,              // Base engine rev volume (%)
    "TURBO": 40,                   // Turbo whistle volume (%)
    "KNOCK": 200,                  // Diesel knock volume (%)
    "WASTEGATE": 100,              // Wastegate/blowoff volume (%)
    "HORN": 100,                   // Horn volume (%)
    "SIREN": 100,                  // Siren volume (%)
    "BRAKE": 150,                  // Air brake volume (%)
    "PARKING_BRAKE": 150,          // Parking brake volume (%)
    "SHIFTING": 100,               // Gear shift volume (%)
    "REVERSING": 70,               // Reversing beep volume (%)
    "INDICATOR": 100,              // Turn signal volume (%)
    "COUPLING": 100,               // Trailer coupling volume (%)
    "JAKEBRAKE": 150,              // Jake brake volume (%)
    "FAN": 0                       // Cooling fan volume (%)
  }
}
```

## 4. Sound Files (`sounds/*.json`)

Audio sample data in JSON format. Also hardware-agnostic.

```json
{
  "sampleRate": 22050,     // Audio sample rate (Hz)
  "sampleCount": 4406,     // Number of samples
  "samples": [0, 4, 14, 15, 16, 18, ...]  // 8-bit PCM data (-128 to 127)
}
```

## 5. Config Loading

### 5.1 ConfigLoader (filesystem utilities)

`src/ConfigLoader.h` - Mounts LittleFS and provides file discovery:

```cpp
ConfigLoader::begin();                           // Mount filesystem
ConfigLoader::listFiles("/", "hardware-");       // Find hardware configs
ConfigLoader::listFiles("/", "vehicle-");        // Find vehicle configs
ConfigLoader::listFiles("/sounds/", "idle-");    // Find idle sounds
```

### 5.2 ConfigParser (structured loading)

`src/ConfigParser.h` - Parses JSON into typed structs:

```cpp
HardwareConfig hwConfig;
VehicleConfig vehicleConfig;

ConfigParser::loadHardwareConfig("/hardware-config.json", hwConfig);
ConfigParser::loadVehicleConfig("/vehicle-ScaniaV8.json", vehicleConfig);
```

### 5.3 HardwareInit (peripheral setup)

`src/HardwareInit.h` - Initializes motors, servos, LEDs:

```cpp
HardwareInit::init(hwConfig);           // Full initialization
HardwareInit::hotReload(hwConfig);      // Re-init without reboot
HardwareInit::stopAll();                // Emergency stop
```

## 6. Board Pin Mapping

Each board defines its own pin mapping in `src/boards/`. See `src/boards/TRACKLINK_V3.h` for the TRACKLINK board:

| Name | Pin | Function |
|------|-----|----------|
| HBRIDGE_A.PWM1 | 13 | Motor A PWM |
| HBRIDGE_A.PWM2 | 14 | Motor A PWM |
| HBRIDGE_A.BEMF | 4 | Motor A BEMF |
| HBRIDGE_B.DIR | 15 | Motor B direction |
| HBRIDGE_B.PWM | 16 | Motor B PWM |
| HBRIDGE_B.BEMF | 5 | Motor B BEMF |
| HBRIDGE.COMMON_EN | 12 | H-Bridge enable |
| L0-L6 | 42,6-11 | LED outputs |
| S1 | 1 | Servo 1 |
| S1 | 2 | Servo 2 |
| AUDIO.I2S_LRC | 17 | I2S word select |
| AUDIO.I2S_BCLK | 18 | I2S bit clock |
| AUDIO.I2S_DIN | 21 | I2S data |
| AUDIO.I2S_SD | 47 | I2S shutdown |
| POWER.ENABLE | 33 | Board enable |
| POWER.BUTTON | 48 | Power button |
| POWER.V_ADC | 2 | Voltage ADC |
| POWER.CHRG_SENS | 34 | Charge detect |

## 7. Complete Boot Sequence

```
1. Mount LittleFS
2. Parse /hardware-config.json → HardwareConfig (user's board setup)
3. Parse /vehicle-*.json → VehicleConfig (vehicle behavior)
4. Initialize hardware (motors, servos, LEDs based on hardware config)
5. Load vehicle profile (sounds into PSRAM)
6. Start audio engine (I2S output)
7. Ready for RC input
```
