/**
 * @file EasyMotor.cpp
 * @brief Brushed DC motor control using MCPWM v5.x.
 */

#include "EasyMotor.h"
#include "common/pwm_utils.h"
#include "common/pin_manager.h"
#include "common/easing_engine.h"
#include <Arduino.h>

#ifdef SOC_MCPWM_SUPPORTED
#include "driver/mcpwm_prelude.h"
#include "common/mcpwm_manager.h"
#endif

EasyMotor::EasyMotor() {
    _config.freq = 20000; // 20kHz default for silent operation
    _config.driverType = DriverType::DRIVER_2PWM;
}

EasyMotor::EasyMotor(uint8_t pin1, uint8_t pin2) : EasyMotor() {
    begin(pin1, pin2);
}

EasyMotor::EasyMotor(uint8_t pin1, uint8_t pin2, uint8_t enablePin) : EasyMotor() {
    begin(pin1, pin2, enablePin);
}

EasyMotor::EasyMotor(DriverType type, uint8_t pin1, uint8_t pin2) : EasyMotor() {
    begin(type, pin1, pin2);
}

EasyMotor::EasyMotor(DriverType type, uint8_t pin1, uint8_t pin2, uint8_t enablePin) : EasyMotor() {
    begin(type, pin1, pin2, enablePin);
}

EasyMotor::~EasyMotor() {
    end();
}

EasyKit::Result EasyMotor::begin(uint8_t pin1, uint8_t pin2, uint8_t enablePin, bool invert) {
    return begin(DriverType::DRIVER_2PWM, pin1, pin2, enablePin, invert);
}

EasyKit::Result EasyMotor::begin(uint8_t pin1, uint8_t pin2, bool invert) {
    return begin(pin1, pin2, 255, invert);
}

EasyKit::Result EasyMotor::begin(DriverType type, uint8_t pin1, uint8_t pin2, uint8_t enablePin, bool invert) {
    _config.driverType = type;
    _config.pwm1Pin = pin1;
    _config.pwm2Pin = pin2;
    _config.enablePin = enablePin;
    _config.inverted = invert;
    return begin(_config);
}

EasyKit::Result EasyMotor::begin(DriverType type, uint8_t pin1, uint8_t pin2, bool invert) {
    return begin(type, pin1, pin2, 255, invert);
}

EasyKit::Result EasyMotor::begin(const EasyKit::EasyMotorConfig& config) {
    _config = config;
    _inverted = _config.inverted;

#ifdef SOC_MCPWM_SUPPORTED
    _slot = EasyKit::MCPWMManager::instance().allocate();
    if (_slot.unit < 0) return EasyKit::Result::ERR_NO_FREE_OPERATOR;

    mcpwm_timer_config_t timer_cfg = {};
    timer_cfg.group_id      = _slot.unit;
    timer_cfg.clk_src       = MCPWM_TIMER_CLK_SRC_DEFAULT;
    timer_cfg.resolution_hz = _pwmResolution;
    timer_cfg.period_ticks  = _pwmResolution / _config.freq;
    timer_cfg.count_mode    = MCPWM_TIMER_COUNT_MODE_UP;
    if (mcpwm_new_timer(&timer_cfg, &_timer) != ESP_OK) return EasyKit::Result::ERR_HW_FAULT;

    mcpwm_operator_config_t oper_cfg = {};
    oper_cfg.group_id = _slot.unit;
    mcpwm_new_operator(&oper_cfg, &_oper);
    mcpwm_operator_connect_timer(_oper, _timer);

    mcpwm_comparator_config_t cmpr_cfg = {};
    cmpr_cfg.flags.update_cmp_on_tez = true;
    mcpwm_new_comparator(_oper, &cmpr_cfg, &_cmprA);
    if (_config.driverType == DriverType::DRIVER_2PWM) {
        mcpwm_new_comparator(_oper, &cmpr_cfg, &_cmprB);
    }

    mcpwm_generator_config_t gen_cfg = {};
    gen_cfg.gen_gpio_num = _config.pwm1Pin;
    mcpwm_new_generator(_oper, &gen_cfg, &_genA);
    
    if (_config.driverType == DriverType::DRIVER_2PWM) {
        gen_cfg.gen_gpio_num = _config.pwm2Pin;
        mcpwm_new_generator(_oper, &gen_cfg, &_genB);
    }

    // Initial actions for PWM Generator A
    mcpwm_generator_set_action_on_timer_event(_genA, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH));
    mcpwm_generator_set_action_on_compare_event(_genA, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, _cmprA, MCPWM_GEN_ACTION_LOW));
    mcpwm_generator_set_force_level(_genA, 0, true);
    
    if (_config.driverType == DriverType::DRIVER_2PWM) {
        mcpwm_generator_set_action_on_timer_event(_genB, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH));
        mcpwm_generator_set_action_on_compare_event(_genB, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, _cmprB, MCPWM_GEN_ACTION_LOW));
        mcpwm_generator_set_force_level(_genB, 0, true);
    }

    mcpwm_timer_enable(_timer);
    mcpwm_timer_start_stop(_timer, MCPWM_TIMER_START_NO_STOP);

    // Claim Pins
    EasyKit::GlobalPinManager::claimPin(_config.pwm1Pin, this);
    if (_config.driverType == DriverType::DRIVER_2PWM) {
        EasyKit::GlobalPinManager::claimPin(_config.pwm2Pin, this);
    } else if (_config.driverType == DriverType::DRIVER_1PWM_1DIR) {
        pinMode(_config.pwm2Pin, OUTPUT);
        EasyKit::GlobalPinManager::claimPin(_config.pwm2Pin, this);
    }

    if (_config.enablePin != 255) {
        pinMode(_config.enablePin, OUTPUT);
        digitalWrite(_config.enablePin, HIGH);
        EasyKit::GlobalPinManager::claimPin(_config.enablePin, this);
    }

    _attached = true;
    return EasyKit::Result::OK;
#else
    return EasyKit::Result::ERR_HW_FAULT;
#endif
}

void EasyMotor::end() {
    if (!_attached) return;
#ifdef SOC_MCPWM_SUPPORTED
    if (_timer) mcpwm_timer_start_stop(_timer, MCPWM_TIMER_STOP_FULL);
    if (_timer) mcpwm_timer_disable(_timer);  // return timer to INIT state before deletion
    if (_genA) mcpwm_del_generator(_genA);
    if (_genB) mcpwm_del_generator(_genB);
    if (_cmprA) mcpwm_del_comparator(_cmprA);
    if (_cmprB) mcpwm_del_comparator(_cmprB);
    if (_oper) mcpwm_del_operator(_oper);
    if (_timer) mcpwm_del_timer(_timer);
    EasyKit::MCPWMManager::instance().release(_slot);
#endif
    EasyKit::GlobalPinManager::releasePin(_config.pwm1Pin, this);
    if (_config.pwm2Pin != 255) EasyKit::GlobalPinManager::releasePin(_config.pwm2Pin, this);
    if (_config.enablePin != 255) EasyKit::GlobalPinManager::releasePin(_config.enablePin, this);
    _attached = false;
}

void EasyMotor::write(float speed) {
    if (!_attached) return;
    _currentSpeed = EasyKit::clampValue(speed, -100.0f, 100.0f);
    _applyDuty();
}

void EasyMotor::stop() {
    write(0.0f);
}

void EasyMotor::brake() {
    _currentSpeed = 0.0f;
#ifdef SOC_MCPWM_SUPPORTED
    if (_config.driverType == DriverType::DRIVER_2PWM) {
        mcpwm_generator_set_force_level(_genA, 1, true);
        mcpwm_generator_set_force_level(_genB, 1, true);
    } else {
        mcpwm_generator_set_force_level(_genA, 1, true);
    }
#endif
}

void EasyMotor::coast() {
    _currentSpeed = 0.0f;
#ifdef SOC_MCPWM_SUPPORTED
    mcpwm_generator_set_force_level(_genA, 0, true);
    if (_genB) mcpwm_generator_set_force_level(_genB, 0, true);
#endif
}

void EasyMotor::_applyDuty() {
    if (!_attached) return;

    // Apply logical inversion
    float speed = _inverted ? -_currentSpeed : _currentSpeed;

    // Apply min/max constraints (on the magnitude)
    float mag = fabsf(speed);
    if (mag > 0.01f) {
        mag = EasyKit::clampValue(mag, _minDuty, _maxDuty);
    }
    
    // Calculate ticks
    uint32_t period = _pwmResolution / _config.freq;
    uint32_t dutyTicks = (uint32_t)(mag / 100.0f * period);

#ifdef SOC_MCPWM_SUPPORTED
    switch (_config.driverType) {
        case DriverType::DRIVER_2PWM:
            if (speed > 0.01f) {
                // Forward: GenA is PWM, GenB is LOW
                mcpwm_comparator_set_compare_value(_cmprA, dutyTicks);
                mcpwm_generator_set_force_level(_genA, (mag > 99.5f) ? 1 : -1, true);
                mcpwm_generator_set_force_level(_genB, 0, true);
            } else if (speed < -0.01f) {
                // Reverse: GenA is LOW, GenB is PWM
                mcpwm_comparator_set_compare_value(_cmprB, dutyTicks);
                mcpwm_generator_set_force_level(_genB, (mag > 99.5f) ? 1 : -1, true);
                mcpwm_generator_set_force_level(_genA, 0, true);
            } else {
                mcpwm_generator_set_force_level(_genA, 0, true);
                mcpwm_generator_set_force_level(_genB, 0, true);
            }
            break;

        case DriverType::DRIVER_1PWM_1DIR:
            // PWM on GenA, Digital on Pin2
            mcpwm_comparator_set_compare_value(_cmprA, dutyTicks);
            mcpwm_generator_set_force_level(_genA, (mag > 99.5f) ? 1 : (mag < 0.01f ? 0 : -1), true);
            if (_config.pwm2Pin != 255) {
                digitalWrite(_config.pwm2Pin, (speed >= 0.0f) ? HIGH : LOW);
            }
            break;

        case DriverType::DRIVER_1PWM:
            // Unidirectional Speed on GenA
            mcpwm_comparator_set_compare_value(_cmprA, (speed > 0.0f) ? dutyTicks : 0);
            mcpwm_generator_set_force_level(_genA, (speed > 99.5f) ? 1 : (speed < 0.01f ? 0 : -1), true);
            break;
    }
#endif
}

void EasyMotor::setFrequency(uint32_t hz) {
    _config.freq = hz;
    if (_timer) mcpwm_timer_set_period(_timer, _pwmResolution / hz);
}

void EasyMotor::setResolution(uint8_t bits) {
    _pwmResolution = 1UL << bits;
}

void EasyMotor::setDirection(bool forward) {
    _inverted = !forward;
    _applyDuty();
}

void EasyMotor::setDeadtime(float micros) {
    _deadtimeUs = micros;
}

void EasyMotor::setMinDuty(float percent) { _minDuty = percent; }
void EasyMotor::setMaxDuty(float percent) { _maxDuty = percent; }

float EasyMotor::getDeadTime() const { return _deadtimeUs; }
float EasyMotor::getMinDuty()  const { return _minDuty; }
float EasyMotor::getMaxDuty()  const { return _maxDuty; }
float EasyMotor::getSpeed()    const { return _currentSpeed; }

void EasyMotor::forward(uint8_t speed) {
    write((float)speed / 255.0f * 100.0f);
}

void EasyMotor::backward(uint8_t speed) {
    write(-(float)speed / 255.0f * 100.0f);
}

void EasyMotor::run(EasyKit::MotorDir dir, uint8_t speed) {
    if (dir == EasyKit::MotorDir::FORWARD) forward(speed);
    else if (dir == EasyKit::MotorDir::BACKWARD) backward(speed);
    else if (dir == EasyKit::MotorDir::BRAKE) brake();
    else coast();
}

void EasyMotor::onPinStolen(uint8_t pin) {
    end();
}
