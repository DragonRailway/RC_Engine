#pragma once
#include "BoardTypes.h"

struct BaseBoard {
    static constexpr const char* NAME = "Generic";

    static constexpr PinEntry    PINS[]    = {};
    static constexpr DriverEntry DRIVERS[] = {};

    struct AUDIO {
        static constexpr uint8_t I2S_LRC  = 0xFF;
        static constexpr uint8_t I2S_BCLK = 0xFF;
        static constexpr uint8_t I2S_DIN  = 0xFF;
        static constexpr uint8_t I2S_SD   = 0xFF;

        static constexpr uint8_t LRC  = I2S_LRC;
        static constexpr uint8_t BCLK = I2S_BCLK;
        static constexpr uint8_t DIN  = I2S_DIN;
        static constexpr uint8_t SD   = I2S_SD;
    };

    struct POWER {
        static constexpr uint8_t POWER_ENABLE = 0xFF;
        static constexpr uint8_t POWER_BUTTON = 0xFF;
        static constexpr uint8_t VOLTAGE_SENS = 0xFF;
        static constexpr uint8_t CHARGE_SENS  = 0xFF;

        static constexpr uint8_t ENABLE   = POWER_ENABLE;
        static constexpr uint8_t BUTTON   = POWER_BUTTON;
        static constexpr uint8_t VOLT_ADC = VOLTAGE_SENS;
        static constexpr uint8_t CHARGING = CHARGE_SENS;
    };
};
