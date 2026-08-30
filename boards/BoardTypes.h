#pragma once
#include <Arduino.h>
#include <cstring>

static constexpr uint8_t NO_PIN = 0xFF;

// Represents a named pin accessible in JSON config (e.g. "L1", "S1", "A1")
struct PinEntry {
    const char* name;
    uint8_t     pin;
};

// Hardware configuration for an H-Bridge motor driver slot
struct DriverPins {
    uint8_t pwm1;
    uint8_t pwm2;     // pwm2 or direction pin
    uint8_t enable;   // 0xFF = none
    uint8_t bemf;     // 0xFF = none
    bool    dualPwm;  // true = 2-PWM, false = PWM+DIR
};

namespace Driver {
    // 1. Dual-PWM Driver (e.g. DRV8833, TC78H660, TMI8260)
    struct DualPWM {
        uint8_t pwm1;
        uint8_t pwm2;
        uint8_t enable = 0xFF;
        uint8_t bemf   = 0xFF;

        constexpr operator DriverPins() const {
            return DriverPins{pwm1, pwm2, enable, bemf, true};
        }
    };

    // 2. PWM + Direction Driver (e.g. Cytron, LMD18200)
    struct PwmDir {
        uint8_t pwm;
        uint8_t dir;
        uint8_t enable = 0xFF;
        uint8_t bemf   = 0xFF;

        constexpr operator DriverPins() const {
            return DriverPins{pwm, dir, enable, bemf, false};
        }
    };
}

// Named driver mapping for JSON config (e.g. "DRIVER_A", "DRIVER_B")
struct DriverEntry {
    const char* name;
    DriverPins  pins;
};
