#pragma once
#include "BoardBase.h"

// Store link: https://www.elecrow.com/tracklink-v3-dragon-railway.html

struct Board_TRACKLINK_V2 : BaseBoard {
    static constexpr const char* NAME = "TRACKLINK_V2";

    // ───────────────────────────────────────────
    // 1. PIN VOCABULARY (JSON string -> GPIO)
    // ───────────────────────────────────────────
    static constexpr PinEntry PINS[] = {
        {"L0", 42}, // BUILTIN LED
        {"L1", 9},
        {"L2", 10},
        {"L3", 11},
        {"L4", 12},
        {"L5", 13},
        {"L6", 14},
        {"S1", 1},
        {"S2", 2},
    };

    // ───────────────────────────────────────────
    // 2. MOTOR DRIVERS
    // ───────────────────────────────────────────
    static constexpr DriverEntry DRIVERS[] = {
        {
            .name = "DRIVER_A",
            .pins = Driver::DualPWM {
                .pwm1   = 6,
                .pwm2   = 7,
                .enable = 5,
                .bemf   = 8
            }
        },
    };

    // ───────────────────────────────────────────
    // 3. AUDIO (I2S DAC)
    // ───────────────────────────────────────────
    struct AUDIO {
        static constexpr uint8_t I2S_LRC  = 15;
        static constexpr uint8_t I2S_BCLK = 16;
        static constexpr uint8_t I2S_DIN  = 17;
        static constexpr uint8_t I2S_SD   = 18;
    };

    // ───────────────────────────────────────────
    // 4. POWER & BATTERY MANAGEMENT
    // ───────────────────────────────────────────
    struct POWER {
        static constexpr uint8_t POWER_ENABLE       = 21;   // set high to enable board
        static constexpr uint8_t POWER_BUTTON       = 38;   // Detect power button press
        static constexpr uint8_t VOLTAGE_SENS       = 4;    // Voltage sense pin
        static constexpr float   VOLTAGE_DIV_R_HIGH = 100.0f;
        static constexpr float   VOLTAGE_DIV_R_LOW  = 100.0f;
        static constexpr float   DIVIDER_RATIO      = computeVoltageDividerRatio(VOLTAGE_DIV_R_HIGH, VOLTAGE_DIV_R_LOW, VOLTAGE_SENS);
        static constexpr uint8_t CHARGE_SENS        = 48;   // Charging status indicator
    };
};
