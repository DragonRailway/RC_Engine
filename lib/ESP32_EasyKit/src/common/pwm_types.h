/**
 * @file pwm_types.h
 * @brief Shared enums, error codes, and config structs for ESP32_EasyKit.
 */
#pragma once

#include <cstdint>

namespace EasyKit {

// ── Error codes ──────────────────────────────────────────────────────────────
enum class Result : int8_t {
    OK                 =  0,
    ERR_NO_FREE_CHANNEL  = -1,  ///< All LEDC channels exhausted
    ERR_NO_FREE_OPERATOR = -2,  ///< All MCPWM operators exhausted
    ERR_ALREADY_ATTACHED = -3,
    ERR_NOT_ATTACHED     = -4,
    ERR_INVALID_PIN      = -5,
    ERR_INVALID_PARAM    = -6,
    ERR_HW_FAULT         = -7,  ///< Peripheral not available on this SoC
};

/// Helper: true if result is OK
inline bool ok(Result r) { return r == Result::OK; }

// ── LEDC resolution presets ──────────────────────────────────────────────────
enum class LEDCResolution : uint8_t {
    Bits8  = 8,
    Bits10 = 10,
    Bits12 = 12,   ///< default
    Bits14 = 14,
};

// ── DC Motor driver types ────────────────────────────────────────────────────
enum class MotorDriverType : uint8_t {
    DRIVER_2PWM       = 0,  ///< Two PWM pins (H-Bridge)
    DRIVER_1PWM_1DIR  = 1,  ///< Speed (PWM) + Direction (Digital)
    DRIVER_1PWM       = 2,  ///< Unidirectional (Single PWM pin)
};

// ── DC Motor direction ───────────────────────────────────────────────────────
enum class MotorDir : uint8_t {
    FORWARD  = 0,
    BACKWARD = 1,
    BRAKE    = 2,
    COAST    = 3,
};

// ── Blink pattern type ───────────────────────────────────────────────────────
enum class BlinkMode : uint8_t {
    SIMPLE,      ///< on_ms / off_ms square wave
    BURST,       ///< N pulses then a long pause
    HEARTBEAT,   ///< double-beat pattern
    MORSE,       ///< arbitrary dot/dash string
    CANDLE,      ///< random flickering candle/fire effect
};

// ── Servo config (passed to EasyServo::attach) ───────────────────────────────────
struct ServoConfig {
    uint16_t minUs     = 500;    ///< µs for minAngle
    uint16_t maxUs     = 2500;   ///< µs for maxAngle
    uint16_t centerUs  = 1500;   ///< µs for 90°
    float    minAngle  = 0.0f;
    float    maxAngle  = 360.0f;
    uint16_t freq      = 50;     ///< Hz
};

// ── DC Motor config ──────────────────────────────────────────────────────────
struct EasyMotorConfig {
    uint8_t  pwm1Pin     = 255;      ///< PWM 1 pin (always Speed PWM)
    uint8_t  pwm2Pin     = 255;      ///< PWM 2 pin (or Direction pin)
    uint8_t  enablePin   = 255;      ///< Optional enable pin (Active HIGH)
    MotorDriverType driverType = MotorDriverType::DRIVER_2PWM;
    bool     inverted    = false;    ///< Initial direction inversion
    uint32_t freq        = 20000;    ///< Hz
    uint8_t  mcpwmUnit   = 0;        ///< 0 or 1
    bool     useDeadTime = false;
    uint16_t deadTimeNs  = 500;      ///< nanoseconds
};

// ── LEDC LED config ──────────────────────────────────────────────────────────
struct LEDConfig {
    uint32_t       freq        = 4000;
    LEDCResolution resolution  = LEDCResolution::Bits12;
    int8_t         channel     = -1;    ///< -1 = auto-assign
    bool           invertOutput = false;
};

} // namespace EasyKit

