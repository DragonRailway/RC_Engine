#pragma once
#include "BoardBase.h"

// PIN definitions for G-Track Ultimate Control Board for RC Models
// Reference: MAX98357 I2S, TMI8260 H-bridge drivers, 6x Low-side LEDs, 8x Servo/PWM, 4x GPIO

struct Board_GTRACK : BaseBoard {
    static constexpr const char* NAME = "GTRACK";

    // ───────────────────────────────────────────
    // 1. PIN VOCABULARY (JSON string -> GPIO)
    // ───────────────────────────────────────────
    static constexpr PinEntry PINS[] = {
        // LED Outputs (Low-side MOSFETs)
        {"L1", 47}, {"L2", 48}, {"L3", 33},
        {"L4", 34}, {"L5", 37}, {"L6", 38},

        // Servo Outputs (5V, up to 4A)
        {"S1", 8},  {"S2", 9},  {"S3", 10}, {"S4", 11},
        {"E1", 12}, {"E2", 13}, {"E3", 14}, {"E4", 15},

        // Extra GPIO (3.3V)
        {"A1", 1},  {"A2", 2},  {"A3", 3},  {"A4", 4},
    };

    // ───────────────────────────────────────────
    // 2. MOTOR DRIVERS (Always enabled / No EN pin)
    // ───────────────────────────────────────────
    static constexpr DriverEntry DRIVERS[] = {
        {
            .name = "DRIVER_A",
            .pins = Driver::DualPWM {
                .pwm1   = 39,
                .pwm2   = 40,
                .enable = 0xFF,
                .bemf   = 5
            }
        },
        {
            .name = "DRIVER_B",
            .pins = Driver::DualPWM {
                .pwm1   = 41,
                .pwm2   = 42,
                .enable = 0xFF,
                .bemf   = 6
            }
        },
    };

    // ───────────────────────────────────────────
    // 3. AUDIO (I2S DAC - MAX98357)
    // ───────────────────────────────────────────
    struct AUDIO {
        static constexpr uint8_t I2S_LRC  = 16;
        static constexpr uint8_t I2S_BCLK = 17;
        static constexpr uint8_t I2S_DIN  = 18;
        static constexpr uint8_t I2S_SD   = 21;
    };

    // ───────────────────────────────────────────
    // 4. POWER MANAGEMENT & ACCESSORIES
    // ───────────────────────────────────────────
    struct POWER : BaseBoard::POWER {
        static constexpr uint8_t POWER_ENABLE       = 35;   // PWR_EN: set HIGH to keep board ON
        static constexpr uint8_t POWER_BUTTON       = 36;   // PWR_BTN: digital read from MCU
        static constexpr uint8_t VOLTAGE_SENS       = 7;    // VSENS: voltage divider 20k/5.1k
        static constexpr float   VOLTAGE_DIV_R_HIGH = 20.0f;
        static constexpr float   VOLTAGE_DIV_R_LOW  = 5.1f;
        static constexpr uint8_t CHARGE_SENS        = 0xFF; // No dedicated charge sense pin
        static constexpr uint8_t SERVO_ENABLE       = 44;   // 5V_EN: pull LOW to disable 5V buck
        static constexpr uint8_t PUMP_ENABLE        = 43;   // OUT: High-side MOSFET for external loads
    };
};
