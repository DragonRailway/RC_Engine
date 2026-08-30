#pragma once

#include <Arduino.h>
#include <cstring>
#include "boards.h"

class PinMapper {
public:
    // Distinct markers for the motor-driver slots (stored in DriveMotor::hardwareId).
    // High values keep them out of the GPIO pin range.
    static constexpr uint8_t DRIVER_A = 0xE1;
    static constexpr uint8_t DRIVER_B = 0xE2;
    static constexpr uint8_t DRIVER_C = 0xE3;
    static constexpr uint8_t DRIVER_D = 0xE4;

    static uint8_t resolve(const char* name) {
        if (!name) return NO_PIN;

        if (strcmp(name, "DRIVER_A") == 0) return DRIVER_A;
        if (strcmp(name, "DRIVER_B") == 0) return DRIVER_B;
        if (strcmp(name, "DRIVER_C") == 0) return DRIVER_C;
        if (strcmp(name, "DRIVER_D") == 0) return DRIVER_D;

        return Board::resolve(name);
    }

    static DriverPins getDriver(const char* name) {
        return Board::getDriver(name);
    }

    static bool isLED(const char* name) {
        if (!name) return false;
        return (name[0] == 'L' && name[1] >= '0' && name[1] <= '9') ||
               (name[0] == 'E' && name[1] >= '1' && name[1] <= '8') ||
               (name[0] == 'S' && name[1] >= '1' && name[1] <= '8') ||
               (name[0] == 'A' && name[1] >= '1' && name[1] <= '8');
    }

    static bool isServo(const char* name) {
        if (!name) return false;
        return (name[0] == 'S' && name[1] >= '1' && name[1] <= '8') ||
               (name[0] == 'E' && name[1] >= '1' && name[1] <= '8') ||
               (name[0] == 'L' && name[1] >= '0' && name[1] <= '9') ||
               (name[0] == 'A' && name[1] >= '1' && name[1] <= '8');
    }

    static bool isDriver(const char* name) {
        if (!name) return false;
        return strncmp(name, "DRIVER_", 7) == 0;
    }
};
