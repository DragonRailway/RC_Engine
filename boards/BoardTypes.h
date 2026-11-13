#pragma once
#include <Arduino.h>
#include <cstring>

static constexpr int8_t NO_PIN = 127;

// Represents a named pin accessible in JSON config (e.g. "L1", "S1", "A1")
// Negative pin values designate active-low / inverted polarity (e.g. -42).
struct PinEntry {
    const char* name;
    int8_t      pin;
};

// Hardware configuration for an H-Bridge motor driver slot
struct DriverPins {
    int8_t pwm1;
    int8_t pwm2;     // pwm2 or direction pin
    int8_t enable;   // NO_PIN = none
    int8_t bemf;     // NO_PIN = none
    int8_t isens;    // NO_PIN = none (current sense / IPROP)
    bool   dualPwm;  // true = 2-PWM, false = PWM+DIR
};

namespace Driver {
    // 1. Dual-PWM Driver (e.g. DRV8833, TC78H660, TMI8260)
    struct DualPWM {
        int8_t pwm1;
        int8_t pwm2;
        int8_t enable = NO_PIN;
        int8_t bemf   = NO_PIN;
        int8_t isens  = NO_PIN;

        constexpr operator DriverPins() const {
            return DriverPins{pwm1, pwm2, enable, bemf, isens, true};
        }
    };

    // 2. PWM + Direction Driver (e.g. Cytron, LMD18200)
    struct PwmDir {
        int8_t pwm;
        int8_t dir;
        int8_t enable = NO_PIN;
        int8_t bemf   = NO_PIN;
        int8_t isens  = NO_PIN;

        constexpr operator DriverPins() const {
            return DriverPins{pwm, dir, enable, bemf, isens, false};
        }
    };
}

// Named driver mapping for JSON config (e.g. "DRIVER_A", "DRIVER_B")
struct DriverEntry {
    const char* name;
    DriverPins  pins;
};
