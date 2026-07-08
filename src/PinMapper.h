#pragma once

#include <Arduino.h>
#include <cstring>

struct HBridgePins {
    uint8_t pwm1;
    uint8_t pwm2;
    uint8_t bemf;
};

class PinMapper {
public:
    static uint8_t resolve(const char* name) {
        if (!name) return 0xFF;

        if (strcmp(name, "L0") == 0) return 42;
        if (strcmp(name, "L1") == 0) return 6;
        if (strcmp(name, "L2") == 0) return 7;
        if (strcmp(name, "L3") == 0) return 8;
        if (strcmp(name, "L4") == 0) return 9;
        if (strcmp(name, "L5") == 0) return 10;
        if (strcmp(name, "L6") == 0) return 11;

        if (strcmp(name, "S1") == 0) return 1;
        if (strcmp(name, "S2") == 0) return 2;

        if (strcmp(name, "HBRIDGE_A") == 0) return 12;
        if (strcmp(name, "HBRIDGE_B") == 0) return 12;

        return 0xFF;
    }

    static HBridgePins getHBridge(const char* name) {
        if (!name) return {0, 0, 0};

        if (strcmp(name, "HBRIDGE_A") == 0) {
            return {13, 14, 4};
        }
        if (strcmp(name, "HBRIDGE_B") == 0) {
            return {16, 15, 5};
        }
        return {0, 0, 0};
    }

    static bool isLED(const char* name) {
        if (!name) return false;
        return name[0] == 'L' && name[1] >= '0' && name[1] <= '6';
    }

    static bool isServo(const char* name) {
        if (!name) return false;
        return name[0] == 'S' && (name[1] == '1' || name[1] == '2');
    }

    static bool isHBridge(const char* name) {
        if (!name) return false;
        return strncmp(name, "HBRIDGE_", 8) == 0;
    }
};
