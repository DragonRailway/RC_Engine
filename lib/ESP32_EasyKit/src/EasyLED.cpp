/**
 * @file EasyLED.cpp
 * @brief EasyLED implementation — multi-pin PWM control, fading, and blink patterns.
 */

#include "EasyLED.h"
#include "common/ledc_manager.h"
#include "common/pwm_utils.h"
#include <Arduino.h>
#include <cmath>
#include <cctype>
#include "common/easing_engine.h"

// ═════════════════════════════════════════════════════════════════════════════
// Construction / Destruction
// ═════════════════════════════════════════════════════════════════════════════

EasyLED::EasyLED() {}

EasyLED::EasyLED(uint8_t pin) {
    if (pin < 64) {
        _pinMask |= (1ULL << pin);
    }
}

EasyLED::~EasyLED() {
    stop();
    end();
}

// ═════════════════════════════════════════════════════════════════════════════
// Setup
// ═════════════════════════════════════════════════════════════════════════════

EasyKit::Result EasyLED::begin(uint8_t pin, const EasyKit::LEDConfig& config) {
    if (pin >= 64) return EasyKit::Result::ERR_INVALID_PIN;

    // Register pin in masks
    _pinMask |= (1ULL << pin);
    if (config.invertOutput) _invertMask |= (1ULL << pin);
    else _invertMask &= ~(1ULL << pin);

    // If we don't have a channel yet, allocate one
    if (_channel < 0) {
        _freq       = config.freq;
        _resolution = static_cast<uint8_t>(config.resolution);

        if (config.channel >= 0) {
            _channel = EasyKit::LEDCManager::instance().allocateChannel(
                pin, _freq, _resolution, config.channel);
        } else {
            _channel = EasyKit::LEDCManager::instance().allocate(
                pin, _freq, _resolution);
        }

        if (_channel < 0) return EasyKit::Result::ERR_NO_FREE_CHANNEL;
    } else {
        // Channel exists, register the pin with the manager
        EasyKit::LEDCManager::instance().allocateChannel(
            pin, _freq, _resolution, _channel);
    }

    // Attach this specific pin to the LEDC hardware
    EasyKit::GlobalPinManager::claimPin(pin, this);
    if (!ledcAttach(pin, _freq, _resolution)) {
        // Cleanup on failure
        _pinMask &= ~(1ULL << pin);
        EasyKit::LEDCManager::instance().release(pin);
        EasyKit::GlobalPinManager::releasePin(pin, this);
        if (_pinMask == 0) _channel = -1;
        return EasyKit::Result::ERR_HW_FAULT;
    }

    _attachedMask |= (1ULL << pin);
    _duty = 0;
    ledcWrite(pin, 0);
    return EasyKit::Result::OK;
}

void EasyLED::end() {
    for (uint8_t i = 0; i < 64; ++i) {
        if (_attachedMask & (1ULL << i)) {
            ledcDetach(i);
            EasyKit::LEDCManager::instance().release(i);
            EasyKit::GlobalPinManager::releasePin(i, this);
        }
    }
    _pinMask      = 0;
    _invertMask   = 0;
    _attachedMask = 0;
    _channel      = -1;
    _duty         = 0;
}

void EasyLED::attach(uint8_t pin) {
    begin(pin);
}

void EasyLED::attach(uint8_t pin, bool out_invert) {
    EasyKit::LEDConfig cfg;
    cfg.freq         = _freq;
    cfg.resolution   = static_cast<EasyKit::LEDCResolution>(_resolution);
    cfg.invertOutput = out_invert;
    if (_channel >= 0) cfg.channel = _channel;
    begin(pin, cfg);
}

void EasyLED::detach(uint8_t pin) {
    if (pin >= 64) return;
    if (_pinMask & (1ULL << pin)) {
        if (_attachedMask & (1ULL << pin)) {
            ledcDetach(pin);
            EasyKit::LEDCManager::instance().release(pin);
            EasyKit::GlobalPinManager::releasePin(pin, this);
            _attachedMask &= ~(1ULL << pin);
        }
        _pinMask &= ~(1ULL << pin);
    }
    
    if (_pinMask == 0) {
        _channel  = -1;
        _duty     = 0;
    }
}

void EasyLED::detach() {
    end();
}

bool EasyLED::isAttached() const {
    return _attachedMask != 0;
}

void EasyLED::onPinStolen(uint8_t pin) {
    // Another component just claimed this GPIO pin physically.
    // We mathematically sever it out of our internal arrays without a freeze!
    if (_pinMask & (1ULL << pin)) {
        if (_attachedMask & (1ULL << pin)) {
            ledcDetach(pin);
            EasyKit::LEDCManager::instance().release(pin);
            _attachedMask &= ~(1ULL << pin);
        }
        _pinMask &= ~(1ULL << pin);
    }
    
    if (_pinMask == 0) {
        _channel  = -1;
        _duty     = 0;
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// Write
// ═════════════════════════════════════════════════════════════════════════════

void EasyLED::write(uint16_t ticks) {
    if (!_ensurePinsAttached()) return;
    _duty = EasyKit::clampValue((uint32_t)ticks, (uint32_t)0, getMaxDuty());
    _applyDuty();
}

void EasyLED::write(float percent) {
    if (!_ensurePinsAttached()) return;
    _duty = EasyKit::percentToRaw(percent, _resolution);
    _applyDuty();
}

void EasyLED::write(bool value) {
    if (value) on();
    else off();
}

void EasyLED::on() {
    if (!_ensurePinsAttached()) return;
    _duty = getMaxDuty();
    _applyDuty();
}

void EasyLED::off() {
    if (!_ensurePinsAttached()) return;
    _duty = 0;
    _applyDuty();
}

// ═════════════════════════════════════════════════════════════════════════════
// Brightness
// ═════════════════════════════════════════════════════════════════════════════

void EasyLED::analogWrite(uint8_t value) {
    if (!_ensurePinsAttached()) return;
    // Keep 8-bit compatibility but use internal raw duty
    _duty = EasyKit::analogToRaw(value, _resolution);
    _applyDuty();
}

void EasyLED::setDuty(uint32_t duty) {
    if (!_ensurePinsAttached()) return;
    _duty = EasyKit::clampValue(duty, (uint32_t)0, getMaxDuty());
    _applyDuty();
}

uint32_t EasyLED::getDuty() const {
    return _duty;
}

float EasyLED::getDutyPercent() const {
    return EasyKit::rawToPercentFloat(_duty, _resolution);
}

// ═════════════════════════════════════════════════════════════════════════════
// Configuration
// ═════════════════════════════════════════════════════════════════════════════

bool EasyLED::setFrequency(uint32_t hz) {
    _freq = hz;
    for (uint8_t i = 0; i < 64; ++i) {
        if (_attachedMask & (1ULL << i)) {
            ledcChangeFrequency(i, _freq, _resolution);
        }
    }
    return true;
}

uint32_t EasyLED::getFrequency() const { return _freq; }

void EasyLED::setResolution(EasyKit::LEDCResolution res) {
    _resolution = static_cast<uint8_t>(res);
    for (uint8_t i = 0; i < 64; ++i) {
        if (_attachedMask & (1ULL << i)) {
            ledcDetach(i);
            ledcAttach(i, _freq, _resolution);
        }
    }
    if (_attachedMask != 0) _applyDuty();
}

void EasyLED::configure(uint32_t hz, uint8_t res) {
    setFrequency(hz);
    setResolution(static_cast<EasyKit::LEDCResolution>(res));
}

// ═════════════════════════════════════════════════════════════════════════════
// Info
// ═════════════════════════════════════════════════════════════════════════════

int8_t   EasyLED::getChannel() const { return _channel; }
uint32_t EasyLED::getMaxDuty() const { return EasyKit::maxDutyForResolution(_resolution); }

uint8_t EasyLED::getPin() const {
    if (_pinMask == 0) return 255;
    for (uint8_t i = 0; i < 64; ++i) {
        if (_pinMask & (1ULL << i)) return i;
    }
    return 255;
}

uint64_t EasyLED::getPinMask() const {
    return _pinMask;
}

// ═════════════════════════════════════════════════════════════════════════════
// Internal: Core
// ═════════════════════════════════════════════════════════════════════════════

bool EasyLED::_ensurePinsAttached() {
    if (_pinMask == 0) return false;
    if (_pinMask == _attachedMask) return true;

    EasyKit::LEDConfig cfg;
    cfg.freq       = _freq;
    cfg.resolution = static_cast<EasyKit::LEDCResolution>(_resolution);

    uint64_t unattached = _pinMask & ~_attachedMask;
    for (uint8_t i = 0; i < 64; ++i) {
        if (unattached & (1ULL << i)) {
            cfg.invertOutput = (_invertMask & (1ULL << i)) != 0;
            if (_channel >= 0) cfg.channel = _channel;

            // Remove from main mask to prevent begin() from ignoring
            _pinMask &= ~(1ULL << i);
            if (begin(i, cfg) != EasyKit::Result::OK) return false;
        }
    }
    return true;
}

void EasyLED::_applyDuty() {
    for (uint8_t i = 0; i < 64; ++i) {
        if (_attachedMask & (1ULL << i)) {
            bool inverted = (_invertMask & (1ULL << i)) != 0;
            uint32_t dutyVal = inverted ? (getMaxDuty() - _duty) : _duty;
            ledcWrite(i, dutyVal);
        }
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// Fade Logic 
// ═════════════════════════════════════════════════════════════════════════════

bool EasyLED::fade(uint32_t startDuty, uint32_t targetDuty, uint32_t durationMs,
                     Curve curve, void (*onComplete)(void*), void* arg) {
    if (!_ensurePinsAttached()) return false;
    stopBlink();

    _fadeStartDuty  = EasyKit::clampValue(startDuty,  (uint32_t)0, getMaxDuty());
    _fadeTargetDuty = EasyKit::clampValue(targetDuty, (uint32_t)0, getMaxDuty());

    if (durationMs == 0) {
        setDuty(_fadeTargetDuty);
        if (onComplete) onComplete(arg);
        return true;
    }

    _fadeDurationMs = durationMs;
    _fadeStartTime  = millis();
    _fadeCurve      = curve;
    _fadeCb         = onComplete;
    _fadeCbArg      = arg;
    _fading         = true;

    setDuty(_fadeStartDuty);
    return true;
}

bool EasyLED::fadeTo(uint32_t targetDuty, uint32_t durationMs,
                       Curve curve, void (*onComplete)(void*), void* arg) {
    return fade(getDuty(), targetDuty, durationMs, curve, onComplete, arg);
}

bool EasyLED::fadeOut(uint32_t durationMs, Curve curve,
                        void (*onComplete)(void*), void* arg) {
    return fade(getDuty(), 0, durationMs, curve, onComplete, arg);
}

bool EasyLED::fadeIn(uint32_t durationMs, Curve curve,
                       void (*onComplete)(void*), void* arg) {
    return fade(0, getMaxDuty(), durationMs, curve, onComplete, arg);
}

// ═════════════════════════════════════════════════════════════════════════════
// Breathing
// ═════════════════════════════════════════════════════════════════════════════

void EasyLED::startBreathing(uint32_t halfCycleMs, Curve curve) {
    _breathing    = true;
    _breathHalfMs = halfCycleMs;
    _breathCurve  = curve;
    _breathDir    = true;
    _breathNext();
}

void EasyLED::stopBreathing() {
    _breathing = false;
    stopFade();
}

bool EasyLED::isBreathing() const { return _breathing; }

// ═════════════════════════════════════════════════════════════════════════════
// Fade State
// ═════════════════════════════════════════════════════════════════════════════

bool EasyLED::isFading() const { return _fading; }

void EasyLED::stopFade() {
    _fading = false;
    _fadeCb = nullptr;
}

// ═════════════════════════════════════════════════════════════════════════════
// Fade — Internal
// ═════════════════════════════════════════════════════════════════════════════

void EasyLED::_updateFade() {
    if (!_fading) return;

    uint32_t elapsed = millis() - _fadeStartTime;
    if (elapsed >= _fadeDurationMs) {
        setDuty(_fadeTargetDuty);
        _fading = false;

        auto cb  = _fadeCb;
        auto arg = _fadeCbArg;
        _fadeCb    = nullptr;
        _fadeCbArg = nullptr;

        if (_breathing) {
            _breathDir = !_breathDir;
            _breathNext();
        }

        if (cb) cb(arg);
        return;
    }

    float t = (float)elapsed / (float)_fadeDurationMs;
    float curved = _applyCurve(t, _fadeCurve);

    int32_t range = (int32_t)_fadeTargetDuty - (int32_t)_fadeStartDuty;
    uint32_t newDuty = _fadeStartDuty + (uint32_t)(curved * (float)range);
    newDuty = EasyKit::clampValue(newDuty, (uint32_t)0, getMaxDuty());
    setDuty(newDuty);
}

float EasyLED::_applyCurve(float t, Curve c) {
    switch (c) {
        case Curve::LINEAR:      return EasyKit::Easing::linear(t);
        case Curve::EASE_IN:     return EasyKit::Easing::quadraticIn(t);
        case Curve::EASE_OUT:    return EasyKit::Easing::quadraticOut(t);
        case Curve::EASE_IN_OUT: return EasyKit::Easing::sigmoid(t, 0.5f); 
        case Curve::SINE:        return EasyKit::Easing::sine(t);
        default:                 return t;
    }
}

void EasyLED::_breathNext() {
    if (_duty == 0) {
        fade(0, getMaxDuty(), _breathHalfMs, _breathCurve);
    } else {
        fade(getMaxDuty(), 0, _breathHalfMs, _breathCurve);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// Blink — Simple
// ═════════════════════════════════════════════════════════════════════════════

void EasyLED::startBlink(uint32_t onMs) {
    startBlink(onMs, onMs, (uint16_t)getMaxDuty());
}

void EasyLED::startBlink(uint32_t onMs, uint32_t offMs) {
    startBlink(onMs, offMs, (uint16_t)getMaxDuty());
}

void EasyLED::startBlink(uint32_t onMs, uint32_t offMs, uint16_t duty) {
    if (!_ensurePinsAttached()) return;
    stopFade();

    _blinkMode    = EasyKit::BlinkMode::SIMPLE;
    _onMs         = onMs;
    _offMs        = offMs;
    _blinkDuty    = EasyKit::clampValue((uint32_t)duty, (uint32_t)0, getMaxDuty());
    _blinkTarget  = 0;
    _blinkCount   = 0;
    _blinkRepeat  = true;
    _blinkRunning = true;
    _blinkOnComplete = nullptr;

    _blinkSetOn();
    _blinkState = _BlinkState::BLINK_ON;
    _lastEvent  = millis();
}

void EasyLED::startBlink(uint32_t onMs, uint32_t offMs, float dutyPercent) {
    startBlink(onMs, offMs, (uint16_t)EasyKit::percentToRaw(dutyPercent, _resolution));
}

void EasyLED::blinkN(uint32_t onMs, uint32_t offMs, uint16_t count,
                       void (*onComplete)(void*), void* arg) {
    if (!_ensurePinsAttached()) return;
    stopFade();

    _blinkMode       = EasyKit::BlinkMode::SIMPLE;
    _onMs            = onMs;
    _offMs           = offMs;
    _blinkDuty       = (uint16_t)getMaxDuty();
    _blinkTarget     = count;
    _blinkCount      = 0;
    _blinkRepeat     = false;
    _blinkRunning    = true;
    _blinkOnComplete    = onComplete;
    _blinkOnCompleteArg = arg;

    _blinkSetOn();
    _blinkState = _BlinkState::BLINK_ON;
    _lastEvent  = millis();
}

// ═════════════════════════════════════════════════════════════════════════════
// Blink — Burst
// ═════════════════════════════════════════════════════════════════════════════

void EasyLED::startBurst(uint32_t onMs, uint32_t offMs, uint32_t pauseMs,
                           uint16_t pulses, uint16_t duty) {
    if (!_ensurePinsAttached()) return;
    stopFade();

    _blinkMode    = EasyKit::BlinkMode::BURST;
    _onMs         = onMs;
    _offMs        = offMs;
    _pauseMs      = pauseMs;
    _blinkDuty    = EasyKit::clampValue((uint32_t)duty, (uint32_t)0, getMaxDuty());
    _pulseTarget  = pulses;
    _pulseCount   = 0;
    _blinkRunning = true;
    _blinkRepeat  = true;

    _blinkSetOn();
    _blinkState = _BlinkState::BURST_ON;
    _lastEvent  = millis();
}

void EasyLED::startBurst(uint32_t onMs, uint32_t offMs, uint32_t pauseMs,
                           uint16_t pulses, float dutyPercent) {
    startBurst(onMs, offMs, pauseMs, pulses, (uint16_t)EasyKit::percentToRaw(dutyPercent, _resolution));
}

// ═════════════════════════════════════════════════════════════════════════════
// Blink — Heartbeat
// ═════════════════════════════════════════════════════════════════════════════

void EasyLED::startHeartbeat(uint32_t beat1Ms, uint32_t beat2Ms,
                               uint32_t beatGapMs, uint32_t pauseMs,
                               uint16_t duty) {
    if (!_ensurePinsAttached()) return;
    stopFade();

    _blinkMode    = EasyKit::BlinkMode::HEARTBEAT;
    _beat1Ms      = beat1Ms;
    _beat2Ms      = beat2Ms;
    _beatGapMs    = beatGapMs;
    _pauseMs      = pauseMs;
    _blinkDuty    = (duty == 0) ? getMaxDuty() : EasyKit::clampValue((uint32_t)duty, (uint32_t)0, getMaxDuty());
    _beatPhase    = 0;
    _blinkRunning = true;
    _blinkRepeat  = true;

    _blinkSetOn();
    _blinkState = _BlinkState::BLINK_ON;
    _lastEvent  = millis();
}

void EasyLED::startHeartbeat(uint32_t beat1Ms, uint32_t beat2Ms,
                               uint32_t beatGapMs, uint32_t pauseMs,
                               float dutyPercent) {
    startHeartbeat(beat1Ms, beat2Ms, beatGapMs, pauseMs, (uint16_t)EasyKit::percentToRaw(dutyPercent, _resolution));
}

// ═════════════════════════════════════════════════════════════════════════════
// Blink — Candle
// ═════════════════════════════════════════════════════════════════════════════

void EasyLED::startCandle(uint8_t speed, uint8_t jitter) {
    if (!_ensurePinsAttached()) return;
    stopFade();

    _blinkMode     = EasyKit::BlinkMode::CANDLE;
    _candleSpeed   = speed;
    _candleJitter  = jitter;
    _candleLastMs  = 0;
    _blinkRunning  = true;
    _blinkState    = _BlinkState::BLINK_ON;
    _blinkDuty     = (uint16_t)getMaxDuty();

    _blinkSetOn();
}

// ═════════════════════════════════════════════════════════════════════════════
// Blink — Morse
// ═════════════════════════════════════════════════════════════════════════════

void EasyLED::startMorse(const char* text, uint32_t ditMs,
                           bool repeat,
                           void (*onComplete)(void*), void* arg) {
    if (!_ensurePinsAttached() || !text || text[0] == '\0') return;
    stopFade();

    _blinkMode          = EasyKit::BlinkMode::MORSE;
    _morseText          = text;
    _morseIdx           = 0;
    _ditMs              = ditMs;
    _blinkRepeat        = repeat;
    _blinkRunning       = true;
    _blinkOnComplete    = onComplete;
    _blinkOnCompleteArg = arg;
    _morseSymbol        = nullptr;
    _morseSymPos        = 0;
    _blinkDuty          = (uint16_t)getMaxDuty();

    _advanceMorse();
}

// ═════════════════════════════════════════════════════════════════════════════
// Blink State
// ═════════════════════════════════════════════════════════════════════════════

void EasyLED::stopBlink() {
    _blinkRunning = false;
    _blinkState   = _BlinkState::IDLE;
    _blinkSetOff();
}

bool EasyLED::isBlinking() const {
    return _blinkRunning;
}

// ═════════════════════════════════════════════════════════════════════════════
// Polling & Global Control
// ═════════════════════════════════════════════════════════════════════════════

void EasyLED::update() {
    _updateFade();
    if (_blinkRunning) _blinkTick();
}

void EasyLED::stop() {
    stopBreathing();
    stopFade();
    stopBlink();
}

// ═════════════════════════════════════════════════════════════════════════════
// Blink — Internal State Machine
// ═════════════════════════════════════════════════════════════════════════════

void EasyLED::_blinkTick() {
    uint32_t now = millis();
    uint32_t elapsed = now - _lastEvent;

    switch (_blinkMode) {

    case EasyKit::BlinkMode::SIMPLE:
        if (_blinkState == _BlinkState::BLINK_ON && elapsed >= _onMs) {
            _blinkSetOff();
            _blinkState = _BlinkState::BLINK_OFF;
            _lastEvent = now;
            _blinkCount++;
            if (_blinkTarget > 0 && _blinkCount >= _blinkTarget) {
                _blinkRunning = false;
                _blinkState   = _BlinkState::IDLE;
                if (_blinkOnComplete) _blinkOnComplete(_blinkOnCompleteArg);
                return;
            }
        } else if (_blinkState == _BlinkState::BLINK_OFF && elapsed >= _offMs) {
            _blinkSetOn();
            _blinkState = _BlinkState::BLINK_ON;
            _lastEvent = now;
        }
        break;

    case EasyKit::BlinkMode::BURST:
        if (_blinkState == _BlinkState::BURST_ON && elapsed >= _onMs) {
            _blinkSetOff();
            _pulseCount++;
            if (_pulseCount >= _pulseTarget) {
                _blinkState = _BlinkState::BURST_PAUSE;
            } else {
                _blinkState = _BlinkState::BURST_OFF;
            }
            _lastEvent = now;
        } else if (_blinkState == _BlinkState::BURST_OFF && elapsed >= _offMs) {
            _blinkSetOn();
            _blinkState = _BlinkState::BURST_ON;
            _lastEvent = now;
        } else if (_blinkState == _BlinkState::BURST_PAUSE && elapsed >= _pauseMs) {
            _pulseCount = 0;
            _blinkSetOn();
            _blinkState = _BlinkState::BURST_ON;
            _lastEvent = now;
        }
        break;

    case EasyKit::BlinkMode::HEARTBEAT:
        if (_blinkState == _BlinkState::BLINK_ON) {
            uint32_t dur = (_beatPhase == 0) ? _beat1Ms : _beat2Ms;
            if (elapsed >= dur) {
                _blinkSetOff();
                _blinkState = _BlinkState::BLINK_OFF;
                _lastEvent = now;
            }
        } else if (_blinkState == _BlinkState::BLINK_OFF) {
            if (_beatPhase == 0 && elapsed >= _beatGapMs) {
                _beatPhase = 1;
                _blinkSetOn();
                _blinkState = _BlinkState::BLINK_ON;
                _lastEvent = now;
            } else if (_beatPhase == 1 && elapsed >= _pauseMs) {
                _beatPhase = 0;
                _blinkSetOn();
                _blinkState = _BlinkState::BLINK_ON;
                _lastEvent = now;
            }
        }
        break;

    case EasyKit::BlinkMode::MORSE:
        if (_blinkState == _BlinkState::MORSE_ON) {
            uint32_t dur = (_morseSymbol && _morseSymbol[_morseSymPos - 1] == '-')
                           ? _ditMs * 3 : _ditMs;
            if (elapsed >= dur) {
                _blinkSetOff();
                if (_morseSymbol && _morseSymbol[_morseSymPos] != '\0') {
                    _blinkState = _BlinkState::MORSE_OFF;
                } else {
                    _morseIdx++;
                    if (_morseText[_morseIdx] == '\0') {
                        if (_blinkRepeat) {
                            _morseIdx = 0;
                            _blinkState = _BlinkState::MORSE_WORD_GAP;
                        } else {
                            _blinkRunning = false;
                            _blinkState = _BlinkState::IDLE;
                            if (_blinkOnComplete) _blinkOnComplete(_blinkOnCompleteArg);
                            return;
                        }
                    } else if (_morseText[_morseIdx] == ' ') {
                        _blinkState = _BlinkState::MORSE_WORD_GAP;
                        _morseIdx++;
                    } else {
                        _blinkState = _BlinkState::MORSE_LETTER_GAP;
                    }
                }
                _lastEvent = now;
            }
        } else if (_blinkState == _BlinkState::MORSE_OFF && elapsed >= _ditMs) {
            _blinkSetOn();
            _morseSymPos++;
            _blinkState = _BlinkState::MORSE_ON;
            _lastEvent = now;
        } else if (_blinkState == _BlinkState::MORSE_LETTER_GAP && elapsed >= _ditMs * 3) {
            _advanceMorse();
        } else if (_blinkState == _BlinkState::MORSE_WORD_GAP && elapsed >= _ditMs * 7) {
            _advanceMorse();
        }
        break;

    case EasyKit::BlinkMode::CANDLE: {
        uint32_t interval = 1UL << _candleSpeed;
        if (elapsed >= interval) {
            _lastEvent = now;
            uint8_t flicker = (_candleJitter > 0) ? (random(0, _candleJitter + 1)) : 0;
            uint8_t brightness = (flicker > 255) ? 0 : (255 - flicker);
            analogWrite(brightness);
        }
        break;
    }

    default:
        break;
    }
}

void EasyLED::_blinkSetOn() {
    setDuty(_blinkDuty);
}

void EasyLED::_blinkSetOff() {
    setDuty(0);
}

void EasyLED::_advanceMorse() {
    while (_morseText[_morseIdx] == ' ') _morseIdx++;

    if (_morseText[_morseIdx] == '\0') {
        if (_blinkRepeat) {
            _morseIdx = 0;
        } else {
            _blinkRunning = false;
            _blinkState = _BlinkState::IDLE;
            if (_blinkOnComplete) _blinkOnComplete(_blinkOnCompleteArg);
            return;
        }
    }

    char c = _morseText[_morseIdx];
    _morseSymbol = _getMorseCode(c);

    if (!_morseSymbol) {
        _morseIdx++;
        _advanceMorse();
        return;
    }

    _morseSymPos = 0;
    _blinkSetOn();
    _morseSymPos = 1;
    _blinkState = _BlinkState::MORSE_ON;
    _lastEvent = millis();
}

// ═════════════════════════════════════════════════════════════════════════════
// Morse Code Table
// ═════════════════════════════════════════════════════════════════════════════

const char* EasyLED::_getMorseCode(char c) {
    c = toupper(c);
    switch (c) {
        case 'A': return ".-";
        case 'B': return "-...";
        case 'C': return "-.-.";
        case 'D': return "-..";
        case 'E': return ".";
        case 'F': return "..-.";
        case 'G': return "--.";
        case 'H': return "....";
        case 'I': return "..";
        case 'J': return ".---";
        case 'K': return "-.-";
        case 'L': return ".-..";
        case 'M': return "--";
        case 'N': return "-.";
        case 'O': return "---";
        case 'P': return ".--.";
        case 'Q': return "--.-";
        case 'R': return ".-.";
        case 'S': return "...";
        case 'T': return "-";
        case 'U': return "..-";
        case 'V': return "...-";
        case 'W': return ".--";
        case 'X': return "-..-";
        case 'Y': return "-.--";
        case 'Z': return "--..";
        case '0': return "-----";
        case '1': return ".----";
        case '2': return "..---";
        case '3': return "...--";
        case '4': return "....-";
        case '5': return ".....";
        case '6': return "-....";
        case '7': return "--...";
        case '8': return "---..";
        case '9': return "----.";
        default:  return nullptr;
    }
}
