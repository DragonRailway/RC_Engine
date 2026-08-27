#pragma once

#include <Arduino.h>
#include <cstring>
#include "boards.h"

struct DriverPins {
    uint8_t pwm1;
    uint8_t pwm2;
    uint8_t bemf;
    uint8_t enable;      // driver enable pin (0xFF if none)
    bool dualPwm;        // true = PWM1/PWM2 driver, false = DIR+PWM driver
};

class PinMapper {
public:
    // Distinct markers for the two motor-driver slots (stored in DriveMotor::hardwareId).
    // High values keep them out of the GPIO pin range.
    static constexpr uint8_t DRIVER_A = 0xE1;
    static constexpr uint8_t DRIVER_B = 0xE2;

    static uint8_t resolve(const char* name) {
        if (!name) return 0xFF;

        if (strcmp(name, "DRIVER_A") == 0) return DRIVER_A;
        if (strcmp(name, "DRIVER_B") == 0) return DRIVER_B;

#ifdef TRACKLINK_V3
        if (strcmp(name, "L0") == 0) return PIN::L0;
        if (strcmp(name, "L1") == 0) return PIN::L1;
        if (strcmp(name, "L2") == 0) return PIN::L2;
        if (strcmp(name, "L3") == 0) return PIN::L3;
        if (strcmp(name, "L4") == 0) return PIN::L4;
        if (strcmp(name, "L5") == 0) return PIN::L5;
        if (strcmp(name, "L6") == 0) return PIN::L6;

        if (strcmp(name, "S1") == 0) return PIN::S1;
        if (strcmp(name, "S2") == 0) return PIN::S2;
#elif defined(MIKRO_V2)
        if (strcmp(name, "L0") == 0) return PIN::L0;
        if (strcmp(name, "L1") == 0) return PIN::L1;
        if (strcmp(name, "L2") == 0) return PIN::L2;
        if (strcmp(name, "L3") == 0) return PIN::L3;
        if (strcmp(name, "L4") == 0) return PIN::L4;
        if (strcmp(name, "L5") == 0) return PIN::L5;
        if (strcmp(name, "L6") == 0) return PIN::L6;
        if (strcmp(name, "L7") == 0) return PIN::L7;
        if (strcmp(name, "L8") == 0) return PIN::L8;

        if (strcmp(name, "S1") == 0) return PIN::S1;
        if (strcmp(name, "S2") == 0) return PIN::S2;
        if (strcmp(name, "S3") == 0) return PIN::S3;
        if (strcmp(name, "S4") == 0) return PIN::S4;
#elif defined(GTRACK)
        if (strcmp(name, "L1") == 0) return PIN::L1;
        if (strcmp(name, "L2") == 0) return PIN::L2;
        if (strcmp(name, "L3") == 0) return PIN::L3;
        if (strcmp(name, "L4") == 0) return PIN::L4;
        if (strcmp(name, "L5") == 0) return PIN::L5;
        if (strcmp(name, "L6") == 0) return PIN::L6;

        if (strcmp(name, "S1") == 0) return PIN::S1;
        if (strcmp(name, "S2") == 0) return PIN::S2;
        if (strcmp(name, "S3") == 0) return PIN::S3;
        if (strcmp(name, "S4") == 0) return PIN::S4;
        if (strcmp(name, "E1") == 0) return PIN::E1;
        if (strcmp(name, "E2") == 0) return PIN::E2;
        if (strcmp(name, "E3") == 0) return PIN::E3;
        if (strcmp(name, "E4") == 0) return PIN::E4;

        if (strcmp(name, "A1") == 0) return PIN::A1;
        if (strcmp(name, "A2") == 0) return PIN::A2;
        if (strcmp(name, "A3") == 0) return PIN::A3;
        if (strcmp(name, "A4") == 0) return PIN::A4;
#endif

        return 0xFF;
    }

    static DriverPins getDriver(const char* name) {
        if (!name) return {0, 0, 0, 0xFF, false};

#ifdef TRACKLINK_V3
        if (strcmp(name, "DRIVER_A") == 0) {
            return {DRIVER::A::PWM1, DRIVER::A::PWM2, DRIVER::A::BEMF, DRIVER::COMMON_EN, true};
        }
        if (strcmp(name, "DRIVER_B") == 0) {
            return {DRIVER::B::PWM, DRIVER::B::DIR, DRIVER::B::BEMF, DRIVER::COMMON_EN, false};
        }
#elif defined(MIKRO_V2)
        if (strcmp(name, "DRIVER_A") == 0) {
            return {DRIVER::A::PWM1, DRIVER::A::PWM2, DRIVER::A::BEMF, DRIVER::A::EN, true};
        }
        if (strcmp(name, "DRIVER_B") == 0) {
            return {DRIVER::B::PWM1, DRIVER::B::PWM2, DRIVER::B::BEMF, DRIVER::B::EN, true};
        }
#elif defined(GTRACK)
        if (strcmp(name, "DRIVER_A") == 0) {
            return {DRIVER::A::PWM1, DRIVER::A::PWM2, DRIVER::A::BEMF, 0xFF, true};
        }
        if (strcmp(name, "DRIVER_B") == 0) {
            return {DRIVER::B::PWM1, DRIVER::B::PWM2, DRIVER::B::BEMF, 0xFF, true};
        }
#endif

        return {0, 0, 0, 0xFF, false};
    }

    static bool isLED(const char* name) {
        if (!name) return false;
        return name[0] == 'L' && name[1] >= '0' && name[1] <= '8';
    }

    static bool isServo(const char* name) {
        if (!name) return false;
        return (name[0] == 'S' && name[1] >= '1' && name[1] <= '4') ||
               (name[0] == 'E' && name[1] >= '1' && name[1] <= '4');
    }

    static bool isDriver(const char* name) {
        if (!name) return false;
        return strncmp(name, "DRIVER_", 7) == 0;
    }
};
