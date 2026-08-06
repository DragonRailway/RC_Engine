#pragma once

#include <Arduino.h>
#include <cstring>
#include "boards.h"

struct HBridgePins {
    uint8_t pwm1;
    uint8_t pwm2;
    uint8_t bemf;
    uint8_t enable;      // bridge enable pin (0xFF if none)
    bool dualPwm;        // true = PWM1/PWM2 bridge, false = DIR+PWM bridge
};

class PinMapper {
public:
    // Distinct markers for the two H-bridge slots (stored in DriveMotor::hardwareId).
    // High values keep them out of the GPIO pin range.
    static constexpr uint8_t BRIDGE_A = 0xE1;
    static constexpr uint8_t BRIDGE_B = 0xE2;

    static uint8_t resolve(const char* name) {
        if (!name) return 0xFF;

        if (strcmp(name, "HBRIDGE_A") == 0) return BRIDGE_A;
        if (strcmp(name, "HBRIDGE_B") == 0) return BRIDGE_B;

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
#endif

        return 0xFF;
    }

    static HBridgePins getHBridge(const char* name) {
        if (!name) return {0, 0, 0, 0xFF, false};

#ifdef TRACKLINK_V3
        if (strcmp(name, "HBRIDGE_A") == 0) {
            return {HBRIDGE::A::PWM1, HBRIDGE::A::PWM2, HBRIDGE::A::BEMF, HBRIDGE::COMMON_EN, true};
        }
        if (strcmp(name, "HBRIDGE_B") == 0) {
            return {HBRIDGE::B::PWM, HBRIDGE::B::DIR, HBRIDGE::B::BEMF, HBRIDGE::COMMON_EN, false};
        }
#elif defined(MIKRO_V2)
        if (strcmp(name, "HBRIDGE_A") == 0) {
            return {HBRIDGE::A::PWM1, HBRIDGE::A::PWM2, HBRIDGE::A::BEMF, HBRIDGE::A::EN, true};
        }
        if (strcmp(name, "HBRIDGE_B") == 0) {
            return {HBRIDGE::B::PWM1, HBRIDGE::B::PWM2, HBRIDGE::B::BEMF, HBRIDGE::B::EN, true};
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
        return name[0] == 'S' && name[1] >= '1' && name[1] <= '4';
    }

    static bool isHBridge(const char* name) {
        if (!name) return false;
        return strncmp(name, "HBRIDGE_", 8) == 0;
    }
};
