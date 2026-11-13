#pragma once
#include "BoardTypes.h"

struct BaseBoard {
    static constexpr const char* NAME = "Generic";

    static constexpr PinEntry    PINS[]    = {};
    static constexpr DriverEntry DRIVERS[] = {};

    static constexpr float computeVoltageDividerRatio(float rHigh, float rLow, int8_t pin = 0) {
        if (pin == NO_PIN) return 0.0f;
        if (rLow > 0.0f) {
            return (rHigh + rLow) / rLow;
        }
        return 2.0f;
    }

    struct AUDIO {
        static constexpr int8_t I2S_LRC  = NO_PIN;
        static constexpr int8_t I2S_BCLK = NO_PIN;
        static constexpr int8_t I2S_DIN  = NO_PIN;
        static constexpr int8_t I2S_SD   = NO_PIN;

        static constexpr int8_t LRC  = I2S_LRC;
        static constexpr int8_t BCLK = I2S_BCLK;
        static constexpr int8_t DIN  = I2S_DIN;
        static constexpr int8_t SD   = I2S_SD;
    };

    struct POWER {
        static constexpr int8_t POWER_ENABLE       = NO_PIN;
        static constexpr int8_t POWER_BUTTON       = NO_PIN;
        static constexpr int8_t VOLTAGE_SENS       = NO_PIN;
        static constexpr float  VOLTAGE_DIV_R_HIGH = 0.0f;
        static constexpr float  VOLTAGE_DIV_R_LOW  = 0.0f;
        static constexpr int8_t CHARGE_SENS        = NO_PIN;
        static constexpr int8_t SERVO_ENABLE       = NO_PIN;
        static constexpr int8_t PUMP_ENABLE        = NO_PIN;

        static constexpr int8_t ENABLE   = POWER_ENABLE;
        static constexpr int8_t BUTTON   = POWER_BUTTON;
        static constexpr int8_t VOLT_ADC = VOLTAGE_SENS;
        static constexpr int8_t CHARGING = CHARGE_SENS;
    };
};
