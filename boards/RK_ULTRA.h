#pragma once
#include "BoardBase.h"

// Hardware definition for RK_ULTRA Control Board
// Reference: Schematic V1.0 (2026-10-06)
// MCU: ESP32-S3 MINI
// Audio: MAX98357 I2S DAC
// Drivers: 2x DRV8231ADSGR Dual-PWM H-Bridges
// IMU: LSM6DSVTR (SPI)
// LED Expander: AW9523 (I2C)

struct Board_RK_ULTRA : BaseBoard {
    static constexpr const char* NAME = "RK_ULTRA";

    // ───────────────────────────────────────────
    // 1. PIN VOCABULARY (JSON string -> GPIO)
    // ───────────────────────────────────────────
    static constexpr PinEntry PINS[] = {
        // Servo Outputs (5V rail)
        {"S1", 8},
        {"S2", 9},
        {"S3", 10},
        {"S4", 11},

        // Expansion / Auxiliary Servo / PWM Outputs (5V rail)
        {"E1", 14},
        {"E2", 41},
        {"E3", 40},
        {"E4", 39},

        // Extra Logic GPIOs (Header H1, 3.3V logic)
        {"A1", 42},
        {"A2", 43},
        {"A3", 44},

        // I2C Expansion Bus (AW9523 / External)
        {"SDA", 37},
        {"SCL", 38},
    };

    // ───────────────────────────────────────────
    // 2. MOTOR DRIVERS (DRV8231ADSGR, Dual-PWM)
    // ───────────────────────────────────────────
    static constexpr DriverEntry DRIVERS[] = {
        {
            .name = "DRIVER_A",
            .pins = Driver::DualPWM {
                .pwm1   = 2,       // INA1 (GPIO 2)
                .pwm2   = 3,       // INA2 (GPIO 3)
                .enable = NO_PIN,  // Hardware VREF tied to 3.3V (Always enabled)
                .bemf   = 5,       // BEMFA (GPIO 5)
                .isens  = 4        // IPROPA (GPIO 4)
            }
        },
        {
            .name = "DRIVER_B",
            .pins = Driver::DualPWM {
                .pwm1   = 12,      // INB1 (GPIO 12)
                .pwm2   = 13,      // INB2 (GPIO 13)
                .enable = NO_PIN,  // Hardware VREF tied to 3.3V (Always enabled)
                .bemf   = 7,       // BEMFB (GPIO 7)
                .isens  = 6        // IPROPB (GPIO 6)
            }
        },
    };

    // ───────────────────────────────────────────
    // 3. AUDIO (I2S DAC - MAX98357AEWL+T)
    // ───────────────────────────────────────────
    struct AUDIO {
        static constexpr int8_t I2S_LRC  = 15;  // I2S_LRCLK (GPIO 15)
        static constexpr int8_t I2S_BCLK = 16;  // I2S_BCLK (GPIO 16)
        static constexpr int8_t I2S_DIN  = 17;  // I2S_DIN (GPIO 17)
        static constexpr int8_t I2S_SD   = 18;  // I2S_MODE / SD_MODE# (GPIO 18)
    };

    // ───────────────────────────────────────────
    // 4. POWER MANAGEMENT & ACCESSORIES
    // ───────────────────────────────────────────
    struct POWER : BaseBoard::POWER {
        static constexpr int8_t POWER_ENABLE       = 35;    // PWR_EN: High keeps board ON (via Q4.1 / Q2)
        static constexpr int8_t POWER_BUTTON       = 36;    // PWR_BTN: Digital read from MCU (SW1 pulls to GND via D2)
        static constexpr int8_t VOLTAGE_SENS       = 1;     // VADC: Battery voltage sense (GPIO 1)
        static constexpr float  VOLTAGE_DIV_R_HIGH = 75.0f; // R13 = 75kΩ
        static constexpr float  VOLTAGE_DIV_R_LOW  = 10.0f; // R14 = 10kΩ
        static constexpr int8_t CHARGE_SENS        = NO_PIN;// No dedicated charging indicator pin
        static constexpr int8_t SERVO_ENABLE       = NO_PIN;// 5V TPS51385 buck auto-enabled with PWR
        static constexpr int8_t PUMP_ENABLE        = 48;    // PWR_OUT_EN: High-side MOSFET Q3 for external power
    };

    // ───────────────────────────────────────────
    // 5. ONBOARD PERIPHERALS (Reference metadata)
    // ───────────────────────────────────────────
    struct IMU {
        static constexpr int8_t INT1 = 21;  // IMU_INT1 (GPIO 21)
        static constexpr int8_t SDO  = 26;  // IMU_SDO (GPIO 26, SPI MISO)
        static constexpr int8_t SDI  = 47;  // IMU_SDI (GPIO 47, SPI MOSI)
        static constexpr int8_t CLK  = 33;  // IMU_CLK (GPIO 33, SPI SCK)
        static constexpr int8_t CS   = 34;  // IMU_CS (GPIO 34, SPI CS)
    };

    struct CURRENT_SENSE {
        static constexpr int8_t DRIVER_A = 4;  // IPROPA (GPIO 4)
        static constexpr int8_t DRIVER_B = 6;  // IPROPB (GPIO 6)
    };

    struct EXPANDER {
        static constexpr int8_t SDA = 37;         // EXP_SDA (GPIO 37)
        static constexpr int8_t SCL = 38;         // EXP_SCL (GPIO 38)
        static constexpr uint8_t I2C_ADDR = 0x5A; // AW9523: AD1=5V(1), AD0=GND(0) -> 0x5A
    };
};
