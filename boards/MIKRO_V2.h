#pragma once
#include "BoardBase.h"

// PIN definitions for Mikro V2 board

struct Board_MIKRO_V2 : BaseBoard {
    static constexpr const char* NAME = "MIKRO_V2";

    // ───────────────────────────────────────────
    // 1. PIN VOCABULARY (JSON string -> GPIO)
    // ───────────────────────────────────────────
    static constexpr PinEntry PINS[] = {
        {"L0", 36},
        {"L1", 38}, {"L2", 39}, {"L3", 40}, {"L4", 41},
        {"L5", 42}, {"L6", 43}, {"L7", 1},  {"L8", 2},
        {"S1", 5},  {"S2", 6},  {"S3", 7},  {"S4", 8},
    };

    // ───────────────────────────────────────────
    // 2. MOTOR DRIVERS (Independent EN: GPIO 17 & GPIO 11)
    // ───────────────────────────────────────────
    static constexpr DriverEntry DRIVERS[] = {
        {
            .name = "DRIVER_A",
            .pins = Driver::DualPWM {
                .pwm1   = 18,
                .pwm2   = 21,
                .enable = 17,
                .bemf   = 9
            }
        },
        {
            .name = "DRIVER_B",
            .pins = Driver::DualPWM {
                .pwm1   = 12,
                .pwm2   = 13,
                .enable = 11,
                .bemf   = 10
            }
        },
    };

    // ───────────────────────────────────────────
    // 3. AUDIO (I2S DAC)
    // ───────────────────────────────────────────
    struct AUDIO {
        static constexpr uint8_t I2S_LRC  = 48;
        static constexpr uint8_t I2S_BCLK = 47;
        static constexpr uint8_t I2S_DIN  = 33;
        static constexpr uint8_t I2S_SD   = 34;
    };

    // ───────────────────────────────────────────
    // 4. POWER & BATTERY MANAGEMENT
    // ───────────────────────────────────────────
    struct POWER : BaseBoard::POWER {
        static constexpr uint8_t POWER_ENABLE       = 15;   // set high to enable board
        static constexpr uint8_t POWER_BUTTON       = 14;   // Detect power button press
        static constexpr uint8_t VOLTAGE_SENS       = 4;    // Voltage sense pin
        static constexpr float   VOLTAGE_DIV_R_HIGH = 100.0f;
        static constexpr float   VOLTAGE_DIV_R_LOW  = 100.0f;
        static constexpr uint8_t CHARGE_SENS        = 35;   // Charging status indicator
    };
};
