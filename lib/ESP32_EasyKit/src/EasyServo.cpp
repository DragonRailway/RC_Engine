/**
 * @file EasyServo.cpp
 * @brief EasyServo class implementation using ESP-IDF MCPWM v5.x API.
 */

#include "EasyServo.h"
#include "common/pwm_utils.h"
#include "common/pin_manager.h"
#include <Arduino.h>
#include "driver/mcpwm_prelude.h"
#include "common/mcpwm_manager.h"
#include "common/easing_engine.h"

// 1 MHz timer resolution → 1 tick = 1 µs, so comparator values written in
// microseconds map directly to pulse width. (10 MHz was a 10× pulse-width bug.)
#define SERVO_TIMER_RESOLUTION_HZ 1000000

uint8_t EasyServo::_instanceCount = 0;

EasyServo::EasyServo() {
    _config.minAngle = 0.0f;
    _config.maxAngle = 360.0f;
}

EasyServo::EasyServo(int pin) : EasyServo() {
    attach(pin);
}

EasyServo::~EasyServo() {
    if (_attached) detach();
}

// ── Arduino-compatible attach overloads ──────────────────────────────────────

int EasyServo::attach(int pin) {
    EasyKit::Result r = attach(pin, _config);
    return (r == EasyKit::Result::OK) ? pin : 0;
}

int EasyServo::attach(int pin, int minUs, int maxUs) {
    EasyKit::ServoConfig cfg = _config;
    cfg.minUs = minUs;
    cfg.maxUs = maxUs;
    EasyKit::Result r = attach(pin, cfg);
    return (r == EasyKit::Result::OK) ? pin : 0;
}

EasyKit::Result EasyServo::attach(int pin, const EasyKit::ServoConfig& config) {
    if (pin < 0 || pin >= 64) return EasyKit::Result::ERR_INVALID_PIN;

    // If already attached to a different pin, detach first
    if (_attached && _pin != pin) {
        detach();
    } else if (_attached && _pin == pin) {
        _config = config;
        _applyDuty();
        return EasyKit::Result::OK;
    }

    _config = config;

#ifdef SOC_MCPWM_SUPPORTED
    mcpwm_timer_config_t timer_cfg = {};
    mcpwm_operator_config_t oper_cfg = {};
    mcpwm_comparator_config_t cmpr_cfg = {};
    mcpwm_generator_config_t gen_cfg = {};

    // Allocate MCPWM group, operator, timer, comparator
    _slot = EasyKit::MCPWMManager::instance().allocate();
    if (_slot.unit < 0) return EasyKit::Result::ERR_NO_FREE_OPERATOR;

    timer_cfg.group_id      = _slot.unit;
    timer_cfg.clk_src       = MCPWM_TIMER_CLK_SRC_DEFAULT;
    timer_cfg.resolution_hz = SERVO_TIMER_RESOLUTION_HZ; // 1 MHz → 1 µs/tick
    timer_cfg.period_ticks  = SERVO_TIMER_RESOLUTION_HZ / _config.freq;
    timer_cfg.count_mode    = MCPWM_TIMER_COUNT_MODE_UP;
    if (mcpwm_new_timer(&timer_cfg, &_timer) != ESP_OK) goto fail;

    oper_cfg.group_id = _slot.unit;
    if (mcpwm_new_operator(&oper_cfg, &_oper) != ESP_OK) goto fail;
    if (mcpwm_operator_connect_timer(_oper, _timer) != ESP_OK) goto fail;

    cmpr_cfg.flags.update_cmp_on_tez = true; 
    if (mcpwm_new_comparator(_oper, &cmpr_cfg, &_cmpr) != ESP_OK) goto fail;

    gen_cfg.gen_gpio_num = pin;
    if (mcpwm_new_generator(_oper, &gen_cfg, &_gen) != ESP_OK) goto fail;

    // Set actions
    mcpwm_generator_set_action_on_timer_event(_gen, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH));
    mcpwm_generator_set_action_on_compare_event(_gen, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, _cmpr, MCPWM_GEN_ACTION_LOW));
    mcpwm_generator_set_force_level(_gen, -1, true);

    EasyKit::GlobalPinManager::claimPin(pin, this);
    _pin = pin;
    _attached = true;
    _instanceCount++;
    _currentUs = _config.centerUs;

    mcpwm_comparator_set_compare_value(_cmpr, _currentUs);
    mcpwm_timer_enable(_timer);
    mcpwm_timer_start_stop(_timer, MCPWM_TIMER_START_NO_STOP);

    return EasyKit::Result::OK;

fail:
    detach();
    return EasyKit::Result::ERR_HW_FAULT;
#else
    return EasyKit::Result::ERR_HW_FAULT;
#endif
}


bool EasyServo::attached() const {
    return _attached;
}

uint64_t EasyServo::attachedPins() const {
    if (!_attached) return 0;
    return (1ULL << _pin);
}

void EasyServo::detach(int pin) {
    if (pin != _pin) return;
    detach();
}

void EasyServo::detach() {
    if (!_attached) return;
    
#ifdef SOC_MCPWM_SUPPORTED
    if (_timer) mcpwm_timer_start_stop(_timer, MCPWM_TIMER_STOP_FULL);
    if (_timer) mcpwm_timer_disable(_timer);
    
    if (_gen)   { mcpwm_del_generator(_gen);    _gen   = nullptr; }
    if (_cmpr)  { mcpwm_del_comparator(_cmpr);  _cmpr  = nullptr; }
    if (_oper)  { mcpwm_del_operator(_oper);    _oper  = nullptr; }
    if (_timer) { mcpwm_del_timer(_timer);      _timer = nullptr; }

    EasyKit::MCPWMManager::instance().release(_slot);
    _slot = { -1, -1 };
#endif

    EasyKit::GlobalPinManager::releasePin(_pin, this);
    _pin = -1;
    _attached = false;
    if (_instanceCount > 0) _instanceCount--;
}

void EasyServo::onPinStolen(uint8_t pin) {
    if (pin == _pin) detach();
}

// ── Position control ─────────────────────────────────────────────────────────

float EasyServo::write(float value, float speed, float kIn, float kOut) {
    if (!_attached) return 0.0f;

    float targetAngle;
    if (value < 500.0f) {
        targetAngle = EasyKit::clampValue(value, _config.minAngle, _config.maxAngle);
    } else {
        targetAngle = usToAngle((int)(value + 0.5f), _config.minUs, _config.maxUs, _config.minAngle, _config.maxAngle);
    }

    // Capture current velocity for momentum hand-off before resetting move state
    if (_moving) {
        update(); 
    } else {
        _vNow = 0.0f;
    }

    _startAngle = _currentAngle;
    _targetAngle = targetAngle;

    float moveSpeed = (speed >= 0.0f) ? speed : _speed;
    float ki = _kIn;
    float ko = _kOut;

    if (kIn >= 0.0f) {
        ki = kIn;
        ko = (kOut < 0.0f) ? kIn : kOut;
    }

    if (moveSpeed <= 0.0f) {
        _currentAngle = _targetAngle;
        _moving = false;
        _vNow = 0.0f;
        _applyDuty();
        return 1.0f;
    }

    _kIn = ki;
    _kOut = ko;
    
    float delta = fabsf(_targetAngle - _startAngle);
    if (delta < 0.01f) {
        _moving = false;
        _vNow = 0.0f;
        return 1.0f;
    }

    _durationMS = (uint32_t)((delta / moveSpeed) * 1000.0f);
    _startTimeMS = millis();
    _moving = true;

    _applyDuty();
    wake();

    return 0.0f;
}

void EasyServo::writeMicroseconds(int us) {
    if (!_attached) return;
    _moving = false;
    _currentUs = EasyKit::clampValue(us, (int)_config.minUs, (int)_config.maxUs);
    _currentAngle = usToAngle(_currentUs, _config.minUs, _config.maxUs, _config.minAngle, _config.maxAngle);
    _applyDuty();
}

void EasyServo::update() {
    if (!_attached || !_moving) {
        _vNow = 0.0f;
        return;
    }

    uint32_t elapsed = millis() - _startTimeMS;
    float lastAngle = _currentAngle;

    if (elapsed >= _durationMS) {
        _currentAngle = _targetAngle;
        _moving = false;
        _vNow = 0.0f;
        _applyDuty();
        return;
    }

    float t = (float)elapsed / (float)_durationMS;
    
    // Apply refined asymmetric sigmoid from unified math library
    float ye = EasyKit::Easing::sigmoid(t, _kIn, _kOut);

    _currentAngle = _startAngle + ye * (_targetAngle - _startAngle);

    // Calculate instantaneous velocity (deg/sec) for momentum hand-off
    // delta_angle / delta_time_in_seconds
    static uint32_t lastUpdate = 0;
    uint32_t now = millis();
    float dt = (float)(now - lastUpdate) / 1000.0f;
    if (dt > 0.0001f) {
        _vNow = (_currentAngle - lastAngle) / dt;
    }
    lastUpdate = now;

    _applyDuty();
}

void EasyServo::stop() {
    _moving = false;
}

void EasyServo::sleep() {
    if (!_attached) return;
    _sleeping = true;
#ifdef SOC_MCPWM_SUPPORTED
    if (_gen) mcpwm_generator_set_force_level(_gen, 0, true);
#endif
}

void EasyServo::wake() {
    if (!_attached) return;
    _sleeping = false;
#ifdef SOC_MCPWM_SUPPORTED
    if (_gen) mcpwm_generator_set_force_level(_gen, -1, true);
#endif
    _applyDuty();
}

void EasyServo::_applyDuty() {
    if (!_attached || _sleeping) return;
    _currentUs = angleToUs(_currentAngle, _config.minUs, _config.maxUs, _config.minAngle, _config.maxAngle);
    
#ifdef SOC_MCPWM_SUPPORTED
    mcpwm_comparator_set_compare_value(_cmpr, _currentUs);
#endif
}

// ── Read back ────────────────────────────────────────────────────────────────

int EasyServo::read() const {
    return (int)(usToAngle(_currentUs, _config.minUs, _config.maxUs, _config.minAngle, _config.maxAngle) + 0.5f);
}

float EasyServo::readAngle() const {
    return usToAngle(_currentUs, _config.minUs, _config.maxUs, _config.minAngle, _config.maxAngle);
}

int EasyServo::readMicroseconds() const {
    return _currentUs;
}

// ── Configuration ────────────────────────────────────────────────────────────

void EasyServo::setEndpoints(int minUs, int maxUs) {
    _config.minUs = minUs;
    _config.maxUs = maxUs;
    _applyDuty();
}

void EasyServo::setEndpoints(int minUs, int maxUs, float minAngle, float maxAngle) {
    _config.minUs = minUs;
    _config.maxUs = maxUs;
    _config.minAngle = minAngle;
    _config.maxAngle = maxAngle;
    _applyDuty();
}

void EasyServo::setMinMaxUs(int minUs, int maxUs) {
    _config.minUs = minUs;
    _config.maxUs = maxUs;
    _applyDuty();
}

void EasyServo::setSpeed(float speed, float kIn, float kOut) {
    if (speed >= 0.0f) _speed = speed;
    if (kIn >= 0.0f) {
        _kIn = kIn;
        _kOut = (kOut < 0.0f) ? kIn : kOut;
    }
}

void EasyServo::setFrequency(uint16_t hz) {
    if (hz == 0) return;
    _config.freq = hz;
#ifdef SOC_MCPWM_SUPPORTED
    // Update the timer period in place — mirrors EasyMotor::setFrequency and
    // preserves any in-progress easing move (no detach/attach teardown).
    if (_timer) mcpwm_timer_set_period(_timer, SERVO_TIMER_RESOLUTION_HZ / hz);
#endif
}

// ── Static utilities ─────────────────────────────────────────────────────────

float EasyServo::usToAngle(int us, int minUs, int maxUs, float minAngle, float maxAngle) {
    float clampedUs = (float)EasyKit::clampValue(us, minUs, maxUs);
    return minAngle + (clampedUs - minUs) * (maxAngle - minAngle) / (float)(maxUs - minUs);
}

int EasyServo::angleToUs(float angle, int minUs, int maxUs, float minAngle, float maxAngle) {
    float clampedAngle = EasyKit::clampValue(angle, minAngle, maxAngle);
    return minUs + (int)((clampedAngle - minAngle) / (maxAngle - minAngle) * (maxUs - minUs) + 0.5f);
}

float EasyServo::usToAngle(int us, int minUs, int maxUs) {
    return usToAngle(us, minUs, maxUs, 0.0f, 180.0f);
}

int EasyServo::angleToUs(float angle, int minUs, int maxUs) {
    return angleToUs(angle, minUs, maxUs, 0.0f, 180.0f);
}

uint8_t EasyServo::numAttached() {
    return _instanceCount;
}
