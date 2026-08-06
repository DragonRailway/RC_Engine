/**
 * @file EasyLED.h
 * @brief LED control class for ESP32_EasyKit.
 *
 * Merges PWM control, hardware/software fading, and non-blocking blink
 * patterns into a single class. One instance per LED pin.
 *
 * Features:
 *   - write(bool) → Digital ON/OFF
 *   - write(int)  → 8-bit PWM duty (0–255)
 *   - Hardware fade (zero CPU, linear only)
 *   - Software fade (easing curves: Sine, EaseIn, etc.)
 *   - Non-blocking blink patterns (simple, burst, heartbeat, candle, Morse)
 *   - Auto-allocation of LEDC channels
 */
#pragma once

#include <cstdint>
#include <vector>
#include <algorithm>
#include "common/pwm_types.h"
#include "common/pin_manager.h"

namespace EasyKit {
    struct PinSettings {
        uint8_t pin;
        bool inverted;
    };
}

class EasyLED : public EasyKit::IPinOwner {
public:
    /// Easing curve types for fading.
    enum class Curve : uint8_t {
        LINEAR,
        EASE_IN,
        EASE_OUT,
        EASE_IN_OUT,
        SINE,
    };

    /// Construct without a pin (must call attach() or begin()).
    EasyLED();

    /// Construct with a pin number (auto-attach on first use).
    explicit EasyLED(uint8_t pin);

    ~EasyLED();

    // ═══════════════════════════════════════════════════════════════════════════
    // ── Setup ────────────────────────────────────────────────────────────────
    // ═══════════════════════════════════════════════════════════════════════════

    /// Attach to a pin with the given config. 
    EasyKit::Result begin(uint8_t pin, const EasyKit::LEDConfig& config = {});

    /// Attach a pin (auto-allocates or joins existing channel).
    void attach(uint8_t pin);

    /// Attach a pin with output inversion control.
    void attach(uint8_t pin, bool out_invert);

    /// Detach and release all channels.
    void end();

    /// Detach a specific pin.
    void detach(uint8_t pin);

    /// Detach all pins.
    void detach();

    /// Check if at least one pin is attached.
    bool isAttached() const;

    // ═══════════════════════════════════════════════════════════════════════════
    // ── Write ────────────────────────────────────────────────────────────────
    // ═══════════════════════════════════════════════════════════════════════════

    /// Digital ON (100%) / OFF (0%) on all pins.
    void write(bool value);
    
    /// Raw duty ticks on all pins (e.g., 0–4095 for 12-bit).
    void write(uint16_t ticks);

    /// Percentage duty cycle (0.0f–100.0f) on all pins.
    void write(float percent);

    /// Full brightness on all pins.
    void on();

    /// Duty = 0 on all pins.
    void off();

    // ═══════════════════════════════════════════════════════════════════════════
    // ── Brightness ───────────────────────────────────────────────────────────
    // ═══════════════════════════════════════════════════════════════════════════

    /// 8-bit analogWrite scale (0–255).
    void analogWrite(uint8_t value);

    /// Raw duty cycle ticks.
    void setDuty(uint32_t duty);
    uint32_t getDuty() const;

    /// Normalized duty percentage (0.0f–100.0f).
    float getDutyPercent() const;

    // ═══════════════════════════════════════════════════════════════════════════
    // ── Configuration ────────────────────────────────────────────────────────
    // ═══════════════════════════════════════════════════════════════════════════

    bool     setFrequency(uint32_t hz);
    uint32_t getFrequency() const;
    void     setResolution(EasyKit::LEDCResolution res);
    void     configure(uint32_t hz, uint8_t res);

    // ═══════════════════════════════════════════════════════════════════════════
    // ── Info ─────────────────────────────────────────────────────────
    // ═══════════════════════════════════════════════════════════════════════════

    int8_t   getChannel() const;
    uint32_t getMaxDuty() const;
    uint8_t  getPin() const;  ///< Returns the lowest attached pin, or 255 if none.
    uint64_t getPinMask() const; ///< Returns bitmask of all attached pins (bit N = GPIO N).

    // ═══════════════════════════════════════════════════════════════════════════
    // ── Fade ─────────────────────────────────────────────────────────────────
    // ═══════════════════════════════════════════════════════════════════════════

    bool fade(uint32_t start, uint32_t target, uint32_t durationMs, Curve curve = Curve::LINEAR,
              void (*onComplete)(void*) = nullptr, void* arg = nullptr);
    bool fadeTo(uint32_t target, uint32_t durationMs, Curve curve = Curve::LINEAR,
                void (*onComplete)(void*) = nullptr, void* arg = nullptr);
    bool fadeIn(uint32_t durationMs, Curve curve = Curve::LINEAR,
                void (*onComplete)(void*) = nullptr, void* arg = nullptr);
    bool fadeOut(uint32_t durationMs, Curve curve = Curve::LINEAR,
                 void (*onComplete)(void*) = nullptr, void* arg = nullptr);

    void startBreathing(uint32_t halfCycleMs, Curve curve = Curve::SINE);
    void stopBreathing();
    bool isBreathing() const;

    bool isFading() const;
    void stopFade();

    // ═══════════════════════════════════════════════════════════════════════════
    // ── Blink ────────────────────────────────────────────────────────────────
    // ═══════════════════════════════════════════════════════════════════════════

    void startBlink(uint32_t onMs);
    void startBlink(uint32_t onMs, uint32_t offMs);
    void startBlink(uint32_t onMs, uint32_t offMs, uint16_t duty);
    void startBlink(uint32_t onMs, uint32_t offMs, float dutyPercent);

    void blinkN(uint32_t onMs, uint32_t offMs, uint16_t count,
                void (*onComplete)(void*) = nullptr, void* arg = nullptr);

    void startBurst(uint32_t onMs, uint32_t offMs, uint32_t pauseMs,
                    uint16_t pulses, uint16_t duty);
    void startBurst(uint32_t onMs, uint32_t offMs, uint32_t pauseMs,
                    uint16_t pulses, float dutyPercent = 100.0f);

    void startHeartbeat(uint32_t beat1Ms = 100, uint32_t beat2Ms = 100,
                        uint32_t beatGapMs = 100, uint32_t pauseMs = 700,
                        uint16_t duty = 0); // 0 = full (default)
    void startHeartbeat(uint32_t beat1Ms, uint32_t beat2Ms,
                        uint32_t beatGapMs, uint32_t pauseMs,
                        float dutyPercent);

    void startCandle(uint8_t speed = 6, uint8_t jitter = 15);

    void startMorse(const char* text, uint32_t ditMs = 120,
                    bool repeat = false,
                    void (*onComplete)(void*) = nullptr, void* arg = nullptr);

    void stopBlink();
    bool isBlinking() const;

    // ═══════════════════════════════════════════════════════════════════════════
    // ── Polling & Global Control ─────────────────────────────────────────────
    // ═══════════════════════════════════════════════════════════════════════════

    void update();
    void stop();

    // ── IPinOwner ────────────────────────────────────────────────────────────
    void onPinStolen(uint8_t pin) override;
    const char* getPeripheralName() const override { return "LEDC"; }

private:
    // ── Core state ───────────────────────────────────────────────────────────
    uint64_t _pinMask     = 0;
    uint64_t _invertMask  = 0;
    uint64_t _attachedMask= 0;
    int8_t   _channel     = -1;
    uint32_t _freq        = 4000;
    uint8_t  _resolution  = 12;
    uint32_t _duty        = 0;
    uint32_t _blinkDuty   = 0; // Raw ticks

    bool _ensurePinsAttached();
    void _applyDuty();

    // ── Fade state ───────────────────────────────────────────────────────────
    bool     _fading       = false;
    uint32_t _fadeStartDuty  = 0;
    uint32_t _fadeTargetDuty = 0;
    uint32_t _fadeDurationMs = 0;
    uint32_t _fadeStartTime  = 0;
    Curve    _fadeCurve      = Curve::LINEAR;
    void     (*_fadeCb)(void*) = nullptr;
    void*    _fadeCbArg        = nullptr;

    bool     _breathing      = false;
    bool     _breathDir      = true;
    uint32_t _breathHalfMs   = 1000;
    Curve    _breathCurve    = Curve::SINE;

    static float _applyCurve(float t, Curve c);
    void _breathNext();
    void _updateFade();

    // ── Blink state ──────────────────────────────────────────────────────────
    enum class _BlinkState : uint8_t {
        IDLE,
        BLINK_ON,
        BLINK_OFF,
        PAUSE,
        BURST_ON,
        BURST_OFF,
        BURST_PAUSE,
        MORSE_ON,
        MORSE_OFF,
        MORSE_LETTER_GAP,
        MORSE_WORD_GAP,
    };

    EasyKit::BlinkMode _blinkMode    = EasyKit::BlinkMode::SIMPLE;
    _BlinkState         _blinkState   = _BlinkState::IDLE;
    uint32_t            _onMs         = 500;
    uint32_t            _offMs        = 500;
    uint32_t            _pauseMs      = 0;
    uint32_t            _lastEvent    = 0;
    uint16_t            _pulseCount   = 0;
    uint16_t            _pulseTarget  = 0;
    uint16_t            _blinkCount   = 0;
    uint16_t            _blinkTarget  = 0;
    // _blinkDuty is now in core state section
    bool                _blinkRunning = false;
    bool                _blinkRepeat  = false;

    uint32_t _beat1Ms   = 100;
    uint32_t _beat2Ms   = 100;
    uint32_t _beatGapMs = 100;
    uint8_t  _beatPhase = 0;

    const char* _morseText   = nullptr;
    uint16_t    _morseIdx    = 0;
    uint32_t    _ditMs       = 120;
    const char* _morseSymbol = nullptr;
    uint8_t     _morseSymPos = 0;

    uint8_t  _candleSpeed  = 6;
    uint8_t  _candleJitter = 15;
    uint32_t _candleLastMs = 0;

    void (*_blinkOnComplete)(void*) = nullptr;
    void*  _blinkOnCompleteArg     = nullptr;

    void _blinkTick();
    void _blinkSetOn();
    void _blinkSetOff();
    void _advanceMorse();
    static const char* _getMorseCode(char c);
};

