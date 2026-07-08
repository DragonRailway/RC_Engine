# Board Definitions

## 1. Overview

Board definitions map hardware pins to logical functions. Each board has a header file in `src/boards/` that defines pin mappings using C++ structs.

## 2. File Structure

```
src/boards/
├── TRACKLINK_V3.h    # Elecrow TrackLink V3 board
└── [NEW_BOARD].h     # New board definitions
```

## 3. Board Definition Format

### 3.1 File Template

```cpp
#pragma once

#include <Arduino.h>

#ifdef BOARD_NAME  // Match build flag from platformio.ini

// ── H-Bridge Motor Control ─────────────────────────────────────
struct HBRIDGE {
  struct A {
    static constexpr uint8_t PWM1 = [PIN];
    static constexpr uint8_t PWM2 = [PIN];
    static constexpr uint8_t BEMF = [PIN];
  };
  struct B {
    static constexpr uint8_t DIR = [PIN];
    static constexpr uint8_t PWM = [PIN];
    static constexpr uint8_t BEMF = [PIN];
  };
  static constexpr uint8_t COMMON_EN = [PIN];
};

// ── LED & Servo Outputs ────────────────────────────────────────
struct PIN {
  static constexpr uint8_t L0 = [PIN];  // Built-in LED
  static constexpr uint8_t L1 = [PIN];
  static constexpr uint8_t L2 = [PIN];
  static constexpr uint8_t L3 = [PIN];
  static constexpr uint8_t L4 = [PIN];
  static constexpr uint8_t L5 = [PIN];
  static constexpr uint8_t L6 = [PIN];
  static constexpr uint8_t S1 = [PIN];  // Servo 1
  static constexpr uint8_t S2 = [PIN];  // Servo 2
};

// ── I2S Audio ──────────────────────────────────────────────────
struct AUDIO {
  static constexpr uint8_t I2S_LRC = [PIN];
  static constexpr uint8_t I2S_BCLK = [PIN];
  static constexpr uint8_t I2S_DIN = [PIN];
  static constexpr uint8_t I2S_SD = [PIN];
};

// ── Power Management ───────────────────────────────────────────
struct POWER {
  static constexpr uint8_t ENABLE = [PIN];
  static constexpr uint8_t BUTTON = [PIN];
  static constexpr uint8_t V_ADC = [PIN];
  static constexpr uint8_t CHRG_SENS = [PIN];
};

#endif
```

## 4. Struct Reference

### 4.1 HBRIDGE

Controls DC motor H-bridge drivers.

| Field | Description | Example |
|-------|-------------|---------|
| `A.PWM1` | Motor A PWM channel 1 | 13 |
| `A.PWM2` | Motor A PWM channel 2 | 14 |
| `A.BEMF` | Motor A back-EMF sensing | 4 |
| `B.DIR` | Motor B direction control | 15 |
| `B.PWM` | Motor B speed control | 16 |
| `B.BEMF` | Motor B back-EMF sensing | 5 |
| `COMMON_EN` | H-Bridge enable (active high) | 12 |

### 4.2 PIN

LED and servo output pins.

| Field | Description | Example |
|-------|-------------|---------|
| `L0` | Built-in/status LED | 42 |
| `L1-L6` | Light channel outputs | 6-11 |
| `S1` | Servo 1 (steering/ESC) | 1 |
| `S2` | Servo 2 (gearbox) | 2 |

### 4.3 AUDIO

I2S audio output pins.

| Field | Description | Example |
|-------|-------------|---------|
| `I2S_LRC` | Word select (LR clock) | 17 |
| `I2S_BCLK` | Bit clock | 18 |
| `I2S_DIN` | Data out (to DAC) | 21 |
| `I2S_SD` | Shutdown control | 47 |

### 4.4 POWER

Power management pins.

| Field | Description | Example |
|-------|-------------|---------|
| `ENABLE` | Board enable (active high) | 33 |
| `BUTTON` | Power button input | 48 |
| `V_ADC` | Battery voltage ADC | 2 |
| `CHRG_SENS` | Charge status detect | 34 |

## 5. PlatformIO Integration

### 5.1 Build Flags

Each board requires a build flag to enable its header:

```ini
[env:BOARD_NAME]
build_flags =
    -D BOARD_NAME
    -D BOOT=0
    -D VSCALE=1.8
    -D VOFFSET=-0.2
    -D LED_FREQ=8000
    -D LED_RES=10
    -D DRV_FREQ=24000
    -D DRV_RES=8
```

### 5.2 Build Flag Definitions

| Flag | Description | Typical |
|------|-------------|---------|
| `-D BOARD_NAME` | Board identifier (matches header) | `-D TRACKLINK_V3` |
| `-D BOOT` | Boot button pin | `-D BOOT=0` |
| `-D VSCALE` | Voltage sensing scale factor | `-D VSCALE=1.8` |
| `-D VOFFSET` | Voltage sensing offset | `-D VOFFSET=-0.2` |
| `-D LED_FREQ` | LED PWM frequency (Hz) | `-D LED_FREQ=8000` |
| `-D LED_RES` | LED PWM resolution (bits) | `-D LED_RES=10` |
| `-D DRV_FREQ` | Motor PWM frequency (Hz) | `-D DRV_FREQ=24000` |
| `-D DRV_RES` | Motor PWM resolution (bits) | `-D DRV_RES=8` |

### 5.3 platformio.ini Example

```ini
[env:MY_NEW_BOARD]
board = esp32-s3-devkitc-1
build_flags =
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DARDUINOJSON_ENABLE_COMMENTS=1
    -DMY_NEW_BOARD
    -DBOOT=0
    -DVSCALE=1.8
    -DVOFFSET=-0.2
    -DLED_FREQ=8000
    -DLED_RES=10
    -DDRV_FREQ=24000
    -DDRV_RES=8
```

## 6. Creating a New Board

### 6.1 Steps

1. Create header file: `src/boards/NEW_BOARD.h`
2. Add build flags to `platformio.ini`
3. Define pin structs matching the template
4. Use `#ifdef NEW_BOARD` guard
5. Test with `pio run -e NEW_BOARD`

### 6.2 Example: New Board

**File: `src/boards/MY_BOARD.h`**

```cpp
#pragma once

#include <Arduino.h>

#ifdef MY_BOARD

struct HBRIDGE {
  struct A {
    static constexpr uint8_t PWM1 = 25;
    static constexpr uint8_t PWM2 = 26;
    static constexpr uint8_t BEMF = 36;
  };
  struct B {
    static constexpr uint8_t DIR = 27;
    static constexpr uint8_t PWM = 14;
    static constexpr uint8_t BEMF = 39;
  };
  static constexpr uint8_t COMMON_EN = 12;
};

struct PIN {
  static constexpr uint8_t L0 = 2;   // Built-in LED
  static constexpr uint8_t L1 = 4;
  static constexpr uint8_t L2 = 5;
  static constexpr uint8_t L3 = 15;
  static constexpr uint8_t L4 = 16;
  static constexpr uint8_t L5 = 17;
  static constexpr uint8_t L6 = 18;
  static constexpr uint8_t S1 = 19;
  static constexpr uint8_t S2 = 21;
};

struct AUDIO {
  static constexpr uint8_t I2S_LRC = 22;
  static constexpr uint8_t I2S_BCLK = 23;
  static constexpr uint8_t I2S_DIN = 32;
  static constexpr uint8_t I2S_SD = 33;
};

struct POWER {
  static constexpr uint8_t ENABLE = 13;
  static constexpr uint8_t BUTTON = 0;
  static constexpr uint8_t V_ADC = 34;
  static constexpr uint8_t CHRG_SENS = 35;
};

#endif
```

**platformio.ini addition:**

```ini
[env:MY_BOARD]
board = esp32-s3-devkitc-1
build_flags =
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DMY_BOARD
    -DBOOT=0
    -DVSCALE=1.8
    -DVOFFSET=-0.2
    -DLED_FREQ=8000
    -DLED_RES=10
    -DDRV_FREQ=24000
    -DDRV_RES=8
```

## 7. Pin Selection Guidelines

### 7.1 ESP32-S3 Pin Types

| Pin Type | Available Pins | Notes |
|----------|----------------|-------|
| GPIO | 0-21, 35-48 | General purpose |
| ADC | 1-10, 11-20 | Analog input |
| Touch | 1-14 | Capacitive touch |
| Strapping | 0, 26, 27, 45, 46 | Boot mode |

### 7.2 Reserved Pins

| Pin | Function | Notes |
|-----|----------|-------|
| 0 | Boot button | Input only |
| 26-27 | PSRAM | If using OPI PSRAM |
| 45-46 | Flash | QIO mode |

### 7.3 Recommendations

- **Motor PWM**: Use pins with PWM capability (GPIO 0-21)
- **I2S**: Use dedicated I2S pins or any GPIO
- **ADC**: Use ADC1 pins (GPIO 1-10) for voltage sensing
- **Avoid**: Strapping pins for outputs

## 8. Hardware Config Reference

Hardware configs reference pins by struct name:

```json
{
  "DRIVE_MOTOR": {
    "HARDWARE": "HBRIDGE_A"   // References HBRIDGE.A struct
  },
  "STEERING_SERVO": {
    "HARDWARE": "S1"          // References PIN.S1
  },
  "LIGHTS": {
    "HEAD_LIGHT": {
      "HARDWARE": "L1"        // References PIN.L1
    }
  }
}
```

### 8.1 Hardware Name Mapping

| Config Value | Struct Reference |
|--------------|------------------|
| `HBRIDGE_A` | `HBRIDGE::A` |
| `HBRIDGE_B` | `HBRIDGE::B` |
| `S1` | `PIN::S1` |
| `S2` | `PIN::S2` |
| `L0-L6` | `PIN::L0-L6` |