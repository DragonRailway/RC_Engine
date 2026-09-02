#pragma once
#include "BoardBase.h"

// Source files: https://oshwlab.com/shreeramlive/tracklink-v3
// Store link: https://www.elecrow.com/tracklink-v3-dragon-railway.html

struct Board_TRACKLINK_V3 : BaseBoard {
    static constexpr const char* NAME = "TRACKLINK_V3";

    // ───────────────────────────────────────────
    // 1. PIN VOCABULARY (JSON string -> GPIO)
    // ───────────────────────────────────────────
    static constexpr PinEntry PINS[] = {
        {"L0", 42}, // BUILTIN LED
        {"L1", 11},
        {"L2", 10},
        {"L3", 9},
        {"L4", 8},
        {"L5", 7},
        {"L6", 6},
        {"S1", 1},
        {"S2", 2},
    };

    // ───────────────────────────────────────────
    // 2. MOTOR DRIVERS (Shared EN on GPIO 12)
    // ───────────────────────────────────────────
    static constexpr DriverEntry DRIVERS[] = {
        {
            .name = "DRIVER_A",
            .pins = Driver::DualPWM {
                .pwm1   = 13,
                .pwm2   = 14,
                .enable = 12,
                .bemf   = 4
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

    // ───────────────────────────────────────────
    // 3. AUDIO (I2S DAC)
    // ───────────────────────────────────────────
    struct AUDIO {
        static constexpr uint8_t I2S_LRC  = 17;
        static constexpr uint8_t I2S_BCLK = 18;
        static constexpr uint8_t I2S_DIN  = 21;
        static constexpr uint8_t I2S_SD   = 47;
    };

    // ───────────────────────────────────────────
    // 4. POWER & BATTERY MANAGEMENT
    // ───────────────────────────────────────────
    struct POWER : BaseBoard::POWER {
        static constexpr uint8_t POWER_ENABLE       = 33;   // set high to enable board
        static constexpr uint8_t POWER_BUTTON       = 48;   // Detect power button press
        static constexpr uint8_t VOLTAGE_SENS       = 2;    // Voltage sense pin
        static constexpr float   VOLTAGE_DIV_R_HIGH = 100.0f;
        static constexpr float   VOLTAGE_DIV_R_LOW  = 100.0f;
        static constexpr uint8_t CHARGE_SENS        = 34;   // Charging status indicator
    };
};
