/**
 * @file EasyServo.h
 * @brief EasyServo class - Arduino-compatible Servo control using MCPWM.
 *
 * Drop-in replacement for the standard Arduino Servo library.
 * Now branded as EasyServo, with Servo alias for compatibility.
 * Uses the MCPWM peripheral for precise servo pulse generation.
 *
 * On chips without MCPWM (ESP32-S2, ESP32-C3, ESP32-C2), attach() will fail.
 */
#pragma once

#include <cstdint>
#include "common/pwm_types.h"
#include "common/pin_manager.h"

#if __has_include("soc/soc_caps.h")
#include "soc/soc_caps.h"
#endif

#ifdef SOC_MCPWM_SUPPORTED
#include "driver/mcpwm_prelude.h"
#include "common/mcpwm_manager.h"
#endif

class EasyServo : public EasyKit::IPinOwner {
public:
    EasyServo();
    EasyServo(int pin);
    ~EasyServo();

    // ── Attachment ───────────────────────────────────────────────────────────
    /// Attach to a pin with default configuration.
    int  attach(int pin);

    /// Attach with custom pulse boundaries.
    int  attach(int pin, int minUs, int maxUs);

    /// Attach with full config struct.
    EasyKit::Result attach(int pin, const EasyKit::ServoConfig& config);

    /// Check if currently attached to at least one pin.
    bool attached() const;

    /// Returns the bitmask of all pins currently attached to this instance.
    uint64_t attachedPins() const;

    /// Detach a specific pin.
    void detach(int pin);

    /// Detach all pins and release hardware resources.
    void detach();

    // ── Position control ─────────────────────────────────────────────────────
    /**
     * @brief Moves to an angle or microsecond position.
     * @param value Angle (0.0f-180.0f) or Microseconds (>= 500.0f).
     * @param speed Optional move speed in deg/s. 0.0f = instantaneous.
     * @param kIn   Easing strength at start (0.0f to 0.99f).
     * @param kOut  Easing strength at end (0.0f to 0.99f).
     * @return Normalized progress (0.0f to 1.0f).
     */
    float write(float value, float speed = -1.0f, float kIn = -1.0f, float kOut = -1.0f);

    /// Write an explicit pulse width in microseconds (Instantaneous).
    void writeMicroseconds(int us);

    /// Update the easing engine. Must be called repeatedly in the loop.
    void update();

    /// Stop the servo at its current position immediately.
    void stop();

    /// Stop the PWM signal to the pins (allow servo to go limp).
    void sleep();

    /// Resume the PWM signal.
    void wake();

    // ── Read back ────────────────────────────────────────────────────────────
    /// Returns the last angle written (mapped to custom range).
    int   read() const;
    float readAngle() const;

    /// Returns the last pulse width in microseconds.
    int  readMicroseconds() const;

    // ── Configuration ────────────────────────────────────────────────────────
    /// Set the pulse boundaries and map them to custom angle bounds.
    void setEndpoints(int minUs, int maxUs);
    void setEndpoints(int minUs, int maxUs, float minAngle, float maxAngle);

    /// Update the min/max pulse range (legacy).
    void setMinMaxUs(int minUs, int maxUs);

    /// Set default speed and easing for this instance.
    void setSpeed(float speed, float kIn = -1.0f, float kOut = -1.0f);

    /// Set PWM frequency (default 50Hz).
    void setFrequency(uint16_t hz);

    // ── Static utilities ─────────────────────────────────────────────────────
    /// Convert microseconds to angle.
    static float usToAngle(int us, int minUs, int maxUs, float minAngle, float maxAngle);
    static float usToAngle(int us, int minUs = 500, int maxUs = 2500);

    /// Convert angle to microseconds.
    static int angleToUs(float angle, int minUs, int maxUs, float minAngle, float maxAngle);
    static int angleToUs(float angle, int minUs = 500, int maxUs = 2500);

    /// Number of currently attached Servo instances.
    static uint8_t numAttached();

    // ── IPinOwner ────────────────────────────────────────────────────────────
    void onPinStolen(uint8_t pin) override;
    const char* getPeripheralName() const override { return "MCPWM_SERVO"; }

private:
#ifdef SOC_MCPWM_SUPPORTED
    mcpwm_timer_handle_t  _timer = nullptr;
    mcpwm_oper_handle_t   _oper  = nullptr;
    mcpwm_cmpr_handle_t   _cmpr  = nullptr;
    mcpwm_gen_handle_t    _gen   = nullptr;
    EasyKit::MCPWMManager::Slot _slot = { -1, -1 };
#endif

    EasyKit::ServoConfig _config;
    int      _pin          = -1;
    int      _currentUs    = 1500;
    bool     _attached     = false;
    bool     _sleeping     = false;

    // ── Animation State ─────────────────────────────────────────────────────
    bool     _moving       = false;
    float    _startAngle   = 90.0f;
    float    _targetAngle  = 90.0f;
    float    _currentAngle = 90.0f;
    uint32_t _startTimeMS  = 0;
    uint32_t _durationMS   = 0;
    float    _kIn          = 0.0f;
    float    _kOut         = 0.0f;
    float    _speed        = 0.0f; 
    float    _vNow         = 0.0f; // Current velocity (deg/sec)

    void _applyDuty();

    static uint8_t _instanceCount;
};

// Global alias for compatibility
using Servo = EasyServo;

namespace EasyKit {
    using EasyServo = ::EasyServo;
}
