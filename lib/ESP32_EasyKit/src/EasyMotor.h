/**
 * @file EasyMotor.h
 * @brief Brushed DC motor control over H-bridge using MCPWM.
 *
 * Supports sign-magnitude and locked-antiphase modes, with optional dead-time.
 * On chips without MCPWM (ESP32-S2, ESP32-C3, ESP32-C2), begin() returns ERR_HW_FAULT.
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

class EasyMotor : public EasyKit::IPinOwner {
public:
    using DriverType = EasyKit::MotorDriverType;

    EasyMotor();
    EasyMotor(uint8_t pin1, uint8_t pin2);
    EasyMotor(uint8_t pin1, uint8_t pin2, uint8_t enablePin);
    EasyMotor(DriverType type, uint8_t pin1, uint8_t pin2);
    EasyMotor(DriverType type, uint8_t pin1, uint8_t pin2, uint8_t enablePin);
    ~EasyMotor();

    // ── Initialization ───────────────────────────────────────────────────────

    EasyKit::Result begin(uint8_t pin1, uint8_t pin2 = 255, uint8_t enablePin = 255, bool invert = false);
    EasyKit::Result begin(uint8_t pin1, uint8_t pin2, bool invert);
    EasyKit::Result begin(DriverType type, uint8_t pin1, uint8_t pin2 = 255, uint8_t enablePin = 255, bool invert = false);
    EasyKit::Result begin(DriverType type, uint8_t pin1, uint8_t pin2, bool invert);
    EasyKit::Result begin(const EasyKit::EasyMotorConfig& config);
    void             end();
    /**
     * @brief Set speed and direction instantaneously.
     * @param speed Target speed -100.0f (Reverse) to 100.0f (Forward).
     */
    void write(float speed);
    
    void stop();    ///< Stop (Alias for write(0))
    void brake();   ///< Immediate active stop
    void coast();   ///< Immediate passive stop

    // ── Configuration ───────────────────────────────────────────────────────
    void setFrequency(uint32_t hz);
    void setResolution(uint8_t bits);
    void setDirection(bool forward);
    void setDeadtime(float micros);
    void setMinDuty(float percent);
    void setMaxDuty(float percent);

    float getDeadTime() const;
    float getMinDuty()  const;
    float getMaxDuty()  const;
    float getSpeed()    const;

    // ── Legacy / Discovery API ──────────────────────────────────────────────
    void run(EasyKit::MotorDir dir, uint8_t speed);
    void forward(uint8_t speed);
    void backward(uint8_t speed);

    // ── IPinOwner ────────────────────────────────────────────────────────────
    void onPinStolen(uint8_t pin) override;
    const char* getPeripheralName() const override { return "MCPWM_MOTOR"; }

private:
#ifdef SOC_MCPWM_SUPPORTED
    mcpwm_timer_handle_t  _timer  = nullptr;
    mcpwm_oper_handle_t   _oper   = nullptr;
    mcpwm_cmpr_handle_t   _cmprA  = nullptr;
    mcpwm_cmpr_handle_t   _cmprB  = nullptr;
    mcpwm_gen_handle_t    _genA   = nullptr;
    mcpwm_gen_handle_t    _genB   = nullptr;
    EasyKit::MCPWMManager::Slot _slot = { -1, -1 };
#endif

    EasyKit::EasyMotorConfig _config;
    
    // Safety & Limits
    float _minDuty    = 0.0f;
    float _maxDuty    = 100.0f;
    float _deadtimeUs = 0.0f;
    bool  _inverted   = false;

    // Current State
    float _currentSpeed = 0.0f;
    bool  _attached     = false;

    void _applyDuty();
    uint32_t _pwmResolution = 10000000; // 10MHz base for higher precision
};
