# RC Engine — Board Definition Guide

This guide explains how to define and add a hardware board (`boards/<BOARD>.h`) for the RC Engine firmware.

Every board file in `boards/` is **100% self-contained** and serves as the single source of truth for the physical hardware: named pins for JSON configs, motor driver wiring, audio DAC connections, and power management.

---

## Table of Contents

1. [Board File Structure](#1-board-file-structure)
2. [Sections in Detail](#2-sections-in-detail)
   - [2.1 Name (`NAME`)](#21-name-name)
   - [2.2 Named Pins (`PINS[]`)](#22-named-pins-pins)
   - [2.3 Motor Drivers (`DRIVERS[]`)](#23-motor-drivers-drivers)
   - [2.4 Audio DAC (`AUDIO`)](#24-audio-dac-audio)
   - [2.5 Power & Peripherals (`POWER`)](#25-power--peripherals-power)
3. [Omitting Unused Hardware](#3-omitting-unused-hardware)
4. [Adding a New Board in 3 Steps](#4-adding-a-new-board-in-3-steps)
5. [Complete Board Examples](#5-complete-board-examples)

---

## 1. Board File Structure

A board header (`boards/<BOARD_NAME>.h`) contains up to four optional sections:

```cpp
#pragma once
#include "BoardBase.h"

struct Board_MY_BOARD : BaseBoard {
    static constexpr const char* NAME = "My Board";

    // 1. Pin vocabulary (Maps JSON strings to GPIOs)
    static constexpr PinEntry PINS[] = { ... };

    // 2. Onboard H-bridge motor drivers
    static constexpr DriverEntry DRIVERS[] = { ... };

    // 3. I2S Audio DAC
    struct AUDIO { ... };

    // 4. Power button, latch, battery ADC
    struct POWER { ... };
};
```

---

## 2. Sections in Detail

### 2.1 Name (`NAME`)
The display name of the board:
```cpp
static constexpr const char* NAME = "TRACKLINK_V3";
```

---

### 2.2 Named Pins (`PINS[]`)
The `PINS[]` table maps human-readable names used in `/hardware-config.json` directly to ESP32 GPIO numbers.

```cpp
static constexpr PinEntry PINS[] = {
    // Builtin status LED
    {"L0", 42},

    // LED outputs (MOSFET channels)
    {"L1", 6},  {"L2", 7},  {"L3", 8},
    {"L4", 9},  {"L5", 10}, {"L6", 11},

    // Servo / PWM outputs
    {"S1", 1},  {"S2", 2},

    // Extra GPIOs
    {"A1", 3},  {"A2", 4},
};
```

- **Pin Flexibility (Interchangeable Usage)**:
  - All ESP32 GPIOs in `PINS[]` can be used interchangeably in JSON configs:
    - `L0` .. `L8` (Lighting channels) can also drive servos/ESCs.
    - `S1` .. `S8` / `E1` .. `E8` (Servo/expansion outputs) can also drive LEDs or accessory outputs.
  - `A1` .. `A4`: 3.3V logic GPIO placeholders for future expansion or custom digital I/O.

---

### 2.3 Motor Drivers (`DRIVERS[]`)
Defines onboard H-Bridge motor drivers using self-documenting initializers:

#### A. Dual-PWM Driver (`Driver::DualPWM`)
For 2-pin H-bridges (DRV8833, TC78H660, TMI8260, TB6612):

```cpp
static constexpr DriverEntry DRIVERS[] = {
    {
        .name = "DRIVER_A",
        .pins = Driver::DualPWM {
            .pwm1   = 13, // Forward PWM pin
            .pwm2   = 14, // Reverse PWM pin
            .enable = 12, // Optional: Sleep/EN pin (defaults to 0xFF)
            .bemf   = 4   // Optional: Back-EMF ADC sensing pin (defaults to 0xFF)
        }
    },
    {
        .name = "DRIVER_B",
        .pins = Driver::DualPWM {
            .pwm1   = 15,
            .pwm2   = 16,
            .enable = 12,
            .bemf   = 5
        }
    },
};
```

#### B. PWM + Direction Driver (`Driver::PwmDir`)
For speed PWM + direction GPIO drivers (Cytron, LMD18200):

```cpp
static constexpr DriverEntry DRIVERS[] = {
    {
        .name = "DRIVER_A",
        .pins = Driver::PwmDir {
            .pwm    = 18, // Speed PWM pin
            .dir    = 19, // Direction GPIO pin
            .enable = 17  // Optional enable pin
        }
    }
};
```

---

### 2.4 Audio DAC (`AUDIO`)
Defines the I2S digital audio pins connected to the DAC (e.g. MAX98357A):

```cpp
struct AUDIO {
    static constexpr uint8_t I2S_LRC  = 17; // Word Select / Clock (LCLK)
    static constexpr uint8_t I2S_BCLK = 18; // Bit Clock (BCLK)
    static constexpr uint8_t I2S_DIN  = 21; // Serial Data (DIN)
    static constexpr uint8_t I2S_SD   = 47; // Shutdown / Enable pin
};
```

---

### 2.5 Power & Peripherals (`POWER`)
Defines power button latching, battery ADC, and power accessory switches:

```cpp
struct POWER {
    static constexpr uint8_t POWER_ENABLE = 33;   // Active HIGH to latch power on
    static constexpr uint8_t POWER_BUTTON = 48;   // Momentary power push-button
    static constexpr uint8_t VOLTAGE_SENS = 4;    // Battery voltage ADC divider
    static constexpr uint8_t CHARGE_SENS  = 34;   // USB charging detect input
    static constexpr uint8_t BUCK_5V_EN   = 44;   // Optional: 5V buck enable pin
    static constexpr uint8_t POWER_OUT    = 43;   // Optional: High-side MOSFET power switch
};
```

---

## 3. Omitting Unused Hardware

If your board does not have motor drivers, audio, or a soft-latch power button, **simply do not write those sections**.

### Example: ESC & Servo Board (No Audio, No Drivers, USB/BEC Powered)
```cpp
#pragma once
#include "BoardBase.h"

struct Board_SERVO_PLANE : BaseBoard {
    static constexpr const char* NAME = "Servo Plane";

    // Only write what physically exists!
    static constexpr PinEntry PINS[] = {
        {"L0", 2},
        {"S1", 4}, {"S2", 5}, {"S3", 6}, {"S4", 7}
    };
};
```
*Audio and motor driver subsystems are automatically disabled at compile time.*

---

## 4. Adding a New Board in 3 Steps

1. **Create the board header** `boards/<BOARD_NAME>.h` inheriting from `BaseBoard`:
   ```cpp
   #pragma once
   #include "BoardBase.h"

   struct Board_MY_BOARD : BaseBoard {
       static constexpr const char* NAME = "MY_BOARD";
       static constexpr PinEntry PINS[] = { ... };
   };
   ```

2. **Add the environment to `platformio.ini`**:
   ```ini
   [env:MY_BOARD]
   board = esp32-s3-devkitc-1
   build_flags =
       ${env.build_flags}
       -D MY_BOARD
   ```

3. **Generate headers**:
   ```bash
   python3 scripts/gen_boards_header.py
   ```
   *(Also generated automatically when building with `pio run -e MY_BOARD`).*

---

## 5. Complete Board Examples

### Example 1: `boards/TRACKLINK_V3.h` (Dual Motor, Shared Enable)
```cpp
#pragma once
#include "BoardBase.h"

struct Board_TRACKLINK_V3 : BaseBoard {
    static constexpr const char* NAME = "TRACKLINK_V3";

    static constexpr PinEntry PINS[] = {
        {"L0", 42},
        {"L1", 6},  {"L2", 7},  {"L3", 8},
        {"L4", 9},  {"L5", 10}, {"L6", 11},
        {"S1", 1},  {"S2", 2},
    };

    static constexpr DriverEntry DRIVERS[] = {
        { "DRIVER_A", Driver::DualPWM { .pwm1 = 13, .pwm2 = 14, .enable = 12, .bemf = 4 } },
        { "DRIVER_B", Driver::DualPWM { .pwm1 = 15, .pwm2 = 16, .enable = 12, .bemf = 5 } },
    };

    struct AUDIO {
        static constexpr uint8_t I2S_LRC  = 17;
        static constexpr uint8_t I2S_BCLK = 18;
        static constexpr uint8_t I2S_DIN  = 21;
        static constexpr uint8_t I2S_SD   = 47;
    };

    struct POWER {
        static constexpr uint8_t POWER_ENABLE = 33;
        static constexpr uint8_t POWER_BUTTON = 48;
        static constexpr uint8_t VOLTAGE_SENS = 4;
        static constexpr uint8_t CHARGE_SENS  = 34;
    };
};
```

---

### Example 2: `boards/MIKRO_V2.h` (Independent Driver Enables)
```cpp
#pragma once
#include "BoardBase.h"

struct Board_MIKRO_V2 : BaseBoard {
    static constexpr const char* NAME = "MIKRO_V2";

    static constexpr PinEntry PINS[] = {
        {"L0", 36},
        {"L1", 38}, {"L2", 39}, {"L3", 40}, {"L4", 41},
        {"L5", 42}, {"L6", 43}, {"L7", 1},  {"L8", 2},
        {"S1", 5},  {"S2", 6},  {"S3", 7},  {"S4", 8},
    };

    static constexpr DriverEntry DRIVERS[] = {
        { "DRIVER_A", Driver::DualPWM { .pwm1 = 18, .pwm2 = 21, .enable = 17, .bemf = 9 } },
        { "DRIVER_B", Driver::DualPWM { .pwm1 = 12, .pwm2 = 13, .enable = 11, .bemf = 10 } },
    };

    struct AUDIO {
        static constexpr uint8_t I2S_LRC  = 48;
        static constexpr uint8_t I2S_BCLK = 47;
        static constexpr uint8_t I2S_DIN  = 33;
        static constexpr uint8_t I2S_SD   = 34;
    };

    struct POWER {
        static constexpr uint8_t POWER_ENABLE = 15;
        static constexpr uint8_t POWER_BUTTON = 14;
        static constexpr uint8_t VOLTAGE_SENS = 4;
        static constexpr uint8_t CHARGE_SENS  = 35;
    };
};
```

---

### Example 3: `boards/GTRACK.h` (Multi-Channel, Always-On Drivers)
```cpp
#pragma once
#include "BoardBase.h"

struct Board_GTRACK : BaseBoard {
    static constexpr const char* NAME = "GTRACK";

    static constexpr PinEntry PINS[] = {
        {"L1", 47}, {"L2", 48}, {"L3", 33},
        {"L4", 34}, {"L5", 37}, {"L6", 38},
        {"S1", 8},  {"S2", 9},  {"S3", 10}, {"S4", 11},
        {"E1", 12}, {"E2", 13}, {"E3", 14}, {"E4", 15},
        {"A1", 1},  {"A2", 2},  {"A3", 3},  {"A4", 4},
    };

    static constexpr DriverEntry DRIVERS[] = {
        { "DRIVER_A", Driver::DualPWM { .pwm1 = 39, .pwm2 = 40, .bemf = 5 } },
        { "DRIVER_B", Driver::DualPWM { .pwm1 = 41, .pwm2 = 42, .bemf = 6 } },
    };

    struct AUDIO {
        static constexpr uint8_t I2S_LRC  = 16;
        static constexpr uint8_t I2S_BCLK = 17;
        static constexpr uint8_t I2S_DIN  = 18;
        static constexpr uint8_t I2S_SD   = 21;
    };

    struct POWER {
        static constexpr uint8_t POWER_ENABLE = 35;
        static constexpr uint8_t POWER_BUTTON = 36;
        static constexpr uint8_t VOLTAGE_SENS = 7;
        static constexpr uint8_t BUCK_5V_EN   = 44;
        static constexpr uint8_t POWER_OUT    = 43;
    };
};
```
