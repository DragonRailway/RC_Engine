#pragma once

#include <Arduino.h>
#include "Config.h"
#include "PinMapper.h"
#include <EasyMotor.h>
#include <EasyServo.h>
#include <EasyLED.h>
#include <EasyLEDGroup.h>


class HardwareInit {

    // A motor output channel: one configurable H-bridge or ESC/servo output
    // with the polarity/duty state captured from its config. s_leftCh is the
    // drive motor (Ackermann) or left track (skid-steer); s_rightCh is the
    // skid-steer right track only (NONE otherwise). The driver/esc pointers
    // select the physical EasyKit output: driveMotor/escServo for the
    // drive/left side, auxMotor/auxServo for the skid right side (the aux
    // work-machine channel is excluded in skid mode).
    struct MotorChannel {
        uint8_t    type = HardwareConfig::DriveMotor::NONE;   // DRIVER | ESC
        uint8_t    direction = HardwareConfig::DriveMotor::FORWARD;
        uint8_t    dutyMin = 20;
        uint8_t    dutyMax = 90;
        uint32_t   frequency = 20000;
        bool       attached = false;
        EasyMotor* driver = nullptr;   // used when type == DRIVER
        EasyServo* esc = nullptr;      // used when type == ESC
    };

public:
    static void init(const HardwareConfig& hw) {
        Serial.println("[HardwareInit] Initializing peripherals...");

        // Animation tunables (global; per-output tuning is a future extension)
        s_easingSpeedDegS = hw.animation.easingSpeedDegS;
        s_easingKIn       = hw.animation.easingKIn;
        s_easingKOut      = hw.animation.easingKOut;
        s_fadeDurationMs  = hw.animation.fadeDurationMs;
        resetBlinkTracking();

        // Motor channels are rebuilt from scratch on every init (hotReload
        // calls stopAll() then init()), so reset both before wiring.
        s_leftCh = MotorChannel();
        s_rightCh = MotorChannel();
        s_drivetrainType = hw.drivetrainType;
        if (hw.drivetrainType == HardwareConfig::SKID_STEER) {
            // Left track on the drive output, right track on the aux output
            // (the aux work-machine channel is excluded in skid mode).
            initChannel(s_leftCh, hw.leftMotor, &driveMotor, &escServo);
            initChannel(s_rightCh, hw.rightMotor, &auxMotor, &auxServo);
        } else {
            initChannel(s_leftCh, hw.driveMotor, &driveMotor, &escServo);
            initSteeringServo(hw.steeringServo);
        }
        initLights(hw.lights);
        initAuxOutputs(hw);

        Serial.println("[HardwareInit] Done");
    }

    static void hotReload(const HardwareConfig& hw) {
        Serial.println("[HardwareInit] Hot-reloading...");
        stopAll();
        init(hw);
    }

    static void stopAll() {
        // Fully cancel animation state first so no blink/fade can strand an
        // output (servo easing is cancelled by detach(), LEDs get stop()).
        steeringServo.stop();
        escServo.stop();
        auxServo.stop();
        stopLightAnimations();

        // Release all EasyKit hardware so hotReload can re-attach cleanly.
        driveMotor.end();
        steeringServo.detach();
        escServo.detach();
        auxServo.detach();
        auxMotor.end();
        auxLed.end();
        headLed.end();
        tailLed.end();
        brakeLed.end();
        turnLLed.end();
        turnRLed.end();
        reversingLed.end();
        s_leftCh.attached = false;
        s_rightCh.attached = false;
        Serial.println("[HardwareInit] Stopped all outputs");
    }

    // Advance every EasyKit animation engine. Must be called every main-loop
    // iteration (from src/main.cpp) so easing moves, fades, and blink patterns
    // progress non-blocking. Also monitors the physical power button.
    static void update(uint16_t buttonHoldS = 4, uint8_t indicatorPin = 0xFF) {
        updatePowerButton(buttonHoldS, indicatorPin);
        steeringServo.update();
        escServo.update();
        auxServo.update();
        auxLed.update();
        headLed.update();
        tailLed.update();
        brakeLed.update();
        turnLLed.update();
        turnRLed.update();
        reversingLed.update();
        ditchLLed.update();
        ditchRLed.update();
        stepLed.update();
        cabLed.update();
        s_ditchGroup.update();
    }

    // ────────────────────────────────────────────────────────────────
    // Power Management API
    // ────────────────────────────────────────────────────────────────
    static void latchPower(uint16_t bootLatchS = 1) {
        if (POWER::POWER_ENABLE == 0xFF || POWER::POWER_BUTTON == 0xFF) {
            Serial.println("[HardwareInit] Board lacks power control pins; skipping latchPower()");
            s_powerLatched = true;
            return;
        }
        pinMode(POWER::POWER_ENABLE, OUTPUT);
        digitalWrite(POWER::POWER_ENABLE, LOW);
        pinMode(POWER::POWER_BUTTON, INPUT);
        s_powerLatched = false;

        // If the power button is not pressed at boot, the board was powered
        // via USB (firmware upload / serial monitor) — latch immediately
        // without requiring the physical hold gesture.
        if (digitalRead(POWER::POWER_BUTTON) == LOW) {
            digitalWrite(POWER::POWER_ENABLE, HIGH);
            s_powerLatched = true;
            Serial.println("[HardwareInit] Power latched ON (USB boot — no button hold required)");
            return;
        }

        uint32_t thresholdMs = (uint32_t)bootLatchS * 1000U;
        uint32_t startMs = millis();
        while (digitalRead(POWER::POWER_BUTTON) == HIGH) {
            if (millis() - startMs >= thresholdMs) {
                digitalWrite(POWER::POWER_ENABLE, HIGH);
                s_powerLatched = true;
                Serial.printf("[HardwareInit] Power latched ON (%us boot hold)\n", (unsigned)bootLatchS);
                break;
            }
            delay(1);
        }
        if (!s_powerLatched) {
            Serial.printf("[HardwareInit] Power button released before %us; power not latched\n", (unsigned)bootLatchS);
        }
    }

    static void updatePowerButton(uint16_t buttonHoldS = 4, uint8_t indicatorPin = 0xFF) {
        if (POWER::POWER_BUTTON == 0xFF) return;

        uint32_t now = millis();
        uint32_t holdMs = (uint32_t)buttonHoldS * 1000U;
        if (digitalRead(POWER::POWER_BUTTON) == HIGH) {
            if (!s_powerButtonHolding) {
                s_powerButtonHolding = true;
                s_powerButtonHoldStart = now;
            }

            // Rapid blink feedback (200ms ON / 200ms OFF) while holding power button
            uint8_t duty = ((now / 200) % 2 == 0) ? 100 : 0;
            if (indicatorPin != 0xFF) {
                setLight(indicatorPin, duty);
            } else {
                setLight(turnLPin, duty);
                setLight(turnRPin, duty);
            }

            if (now - s_powerButtonHoldStart >= holdMs) {
                Serial.printf("[HardwareInit] %us button hold detected -> powerOff()\n", (unsigned)buttonHoldS);
                s_powerButtonHolding = false;
                s_powerButtonHoldStart = 0;
                if (indicatorPin != 0xFF) {
                    setLight(indicatorPin, 0);
                } else {
                    setLight(turnLPin, 0);
                    setLight(turnRPin, 0);
                }
                powerOff();
            }
        } else {
            if (s_powerButtonHolding) {
                if (now - s_powerButtonHoldStart < holdMs) {
                    s_buttonClicked = true;
                }
                s_powerButtonHolding = false;
                s_powerButtonHoldStart = 0;
            }
        }
    }

    static bool consumeButtonClicked() {
        bool clicked = s_buttonClicked;
        s_buttonClicked = false;
        return clicked;
    }

    static bool isCharging() {
        if (POWER::CHARGE_SENS == 0xFF) return false;
        return digitalRead(POWER::CHARGE_SENS) == HIGH;
    }

    static void powerOff() {
        Serial.println("[HardwareInit] Powering off hardware...");
        stopAll();
        if (POWER::POWER_ENABLE != 0xFF) {
            digitalWrite(POWER::POWER_ENABLE, LOW);
        }
        s_powerLatched = false;
    }

    static bool isPowerLatched() { return s_powerLatched; }


    static void setSkidMotors(int16_t leftSpeed, int16_t rightSpeed) {
        setChannel(s_leftCh, leftSpeed);
        setChannel(s_rightCh, rightSpeed);
    }

    // Zero/command every configured motor channel (drive/left + skid right).
    // Used by the safety paths (engine OFF/STARTING, battery cutoff) so no
    // track can keep its last commanded speed when the vehicle is not drivable.
    static void setAllMotors(int16_t speed) {
        setChannel(s_leftCh, speed);
        setChannel(s_rightCh, speed);
    }

    // ────────────────────────────────────────────────────────────────
    // Runtime control API (used by VehicleController)
    // ────────────────────────────────────────────────────────────────

    // speed: -100 (full reverse) .. +100 (full forward)
    static void setMotor(int16_t speed) {
        setChannel(s_leftCh, speed);
    }

    // Drive one motor channel at speed (-100..+100). Applies the channel's
    // configured polarity (direction), duty window (duty.min..max), and
    // electrical kind (H-bridge duty vs ESC/PPM pulse). Shared by the
    // Ackermann drive motor and both skid-steer tracks.
    static void setChannel(MotorChannel& ch, int16_t speed) {
        if (ch.type == HardwareConfig::DriveMotor::NONE || !ch.attached) return;

        int16_t eff = speed;
        switch (ch.direction) {
            case HardwareConfig::DriveMotor::REVERSE:     eff = -eff;      break;
            case HardwareConfig::DriveMotor::UNI_FORWARD: eff = abs(eff);  break;
            case HardwareConfig::DriveMotor::UNI_REVERSE: eff = -abs(eff); break;
            default: break;   // FORWARD
        }

        // Duty percent within the configured min..max window; 0 when neutral
        uint8_t pct = 0;
        if (eff > 0) {
            pct = ch.dutyMin + (uint8_t)((uint32_t)eff * (ch.dutyMax - ch.dutyMin) / 100);
        } else if (eff < 0) {
            pct = ch.dutyMin + (uint8_t)((uint32_t)(-eff) * (ch.dutyMax - ch.dutyMin) / 100);
        }
        if (pct > ch.dutyMax) pct = ch.dutyMax;

        if (ch.type == HardwareConfig::DriveMotor::ESC) {
            // PPM pulse: 1000..2000us, neutral 1500us, deadband around center
            uint16_t us = 1500;
            if (abs(eff) >= 5) us = (uint16_t)(1500 + (int32_t)eff * 500 / 100);
            if (us < 1000) us = 1000;
            if (us > 2000) us = 2000;
            if (ch.esc) ch.esc->writeMicroseconds(us);
            return;
        }

        // Motor driver via EasyMotor: signed percent, direction handled by the driver
        if (ch.driver) ch.driver->write(eff >= 0 ? (float)pct : -(float)pct);
    }

    // position: -100 (left) .. +100 (right), 0 = center
    static int16_t s_lastServoPos;
    static void setServo(int16_t position) {
        if (servoPin == 0xFF || !steeringServo.attached()) return;
        if (position == s_lastServoPos) return;
        s_lastServoPos = position;

        int32_t us = servoCenter;
        if (position > 0)      us = servoCenter + (int32_t)position * (servoRight - servoCenter) / 100;
        else if (position < 0) us = servoCenter + (int32_t)position * (servoCenter - servoLeft) / 100;
        if (us < 500)  us = 500;
        if (us > 2500) us = 2500;

        steeringServo.writeMicroseconds(us);
    }

    // brightnessPct: 0..100 (10-bit LED channels)
    static void setLight(uint8_t pin, uint8_t brightnessPct) {
        if (pin == 0xFF || pin == 0) return;
        EasyLED* led = findLight(pin);
        if (led) led->write((float)brightnessPct);
    }

    // Edge-triggered blink control: a false->true edge starts the blink engine
    // (honoring the config interval/duty), a true->false edge stops it and turns
    // the light off. Repeated calls with an unchanged state are no-ops, so the
    // caller can drive this every loop iteration. The pin's blink engine owns
    // its duty while active — no setLight() may target it during a blink.
    static void setLightBlink(uint8_t pin, bool active, uint16_t onMs, uint16_t offMs, uint8_t dutyPct) {
        if (pin == 0xFF || pin == 0) return;
        EasyLED* led = findLight(pin);
        if (!led) return;

        int8_t slot = blinkSlot(pin);
        if (slot < 0) return;

        if (active && !s_blinkActive[slot]) {
            s_blinkActive[slot] = true;
            led->startBlink(onMs, offMs, (float)dutyPct);
        } else if (!active && s_blinkActive[slot]) {
            s_blinkActive[slot] = false;
            led->stopBlink();
        }
    }

    // Fade a light to a target duty percent over durationMs using an ease-in-out
    // curve (starts from the light's current duty, cancelling any active blink).
    static void setLightFade(uint8_t pin, uint8_t targetPct, uint16_t durationMs) {
        if (pin == 0xFF || pin == 0) return;
        EasyLED* led = findLight(pin);
        if (!led) return;
        uint32_t targetTicks = (uint32_t)led->getMaxDuty() * targetPct / 100;
        led->fadeTo(targetTicks, durationMs, EasyLED::Curve::EASE_IN_OUT);
    }

    // Live duty read-back (0..100) of a light, for consumers that track another
    // light's actual brightness (e.g. tail following the headlight's fade).
    static uint8_t getLightDutyPercent(uint8_t pin) {
        if (pin == 0xFF || pin == 0) return 0;
        EasyLED* led = findLight(pin);
        if (!led) return 0;
        return (uint8_t)led->getDutyPercent();
    }

    // Cancel blink/fade/breathing on every attached LED without detaching them
    // (lights stay usable afterwards). Used by battery-cutoff so a hazard or
    // turn blink can never strand an LED on.
    static void stopLightAnimations() {
        headLed.stop();
        tailLed.stop();
        brakeLed.stop();
        turnLLed.stop();
        turnRLed.stop();
        reversingLed.stop();
        ditchLLed.stop();
        ditchRLed.stop();
        stepLed.stop();
        cabLed.stop();
        auxLed.stop();
        s_ditchGroup.stop();
        s_ditchActive = false;
        resetBlinkTracking();
    }

    // Edge-triggered ditch light control: active starts alternate pattern, !active stops it.
    static void setDitchLights(bool active, uint16_t intervalMs) {
        if (ditchLPin == 0xFF && ditchRPin == 0xFF) return;
        if (active && !s_ditchActive) {
            s_ditchActive = true;
            s_ditchGroup.alternate(intervalMs);
        } else if (!active && s_ditchActive) {
            s_ditchActive = false;
            s_ditchGroup.stop();
        }
    }


    // Aux motor: config-driven output channel. The hardware token decided at
    // init whether this drives an H-bridge (EasyMotor) or a servo/ESC PPM
    // output (EasyServo); speed semantics match setMotor (-100..+100).
    static void setAuxMotor(int16_t speed) {
        if (auxMotorType == HardwareConfig::DriveMotor::NONE || !auxMotorAttached) return;

        int16_t eff = speed;
        switch (auxMotorDirection) {
            case HardwareConfig::DriveMotor::REVERSE:    eff = -eff;     break;
            case HardwareConfig::DriveMotor::UNI_FORWARD: eff = abs(eff); break;
            case HardwareConfig::DriveMotor::UNI_REVERSE: eff = -abs(eff); break;
            default: break;   // FORWARD
        }

        // Duty percent within configured min..max window; 0 when neutral
        uint8_t pct = 0;
        if (eff > 0) {
            pct = auxMotorDutyMin + (uint8_t)((uint32_t)eff * (auxMotorDutyMax - auxMotorDutyMin) / 100);
        } else if (eff < 0) {
            pct = auxMotorDutyMin + (uint8_t)((uint32_t)(-eff) * (auxMotorDutyMax - auxMotorDutyMin) / 100);
        }
        if (pct > auxMotorDutyMax) pct = auxMotorDutyMax;

        if (auxMotorType == HardwareConfig::DriveMotor::ESC) {
            // PPM pulse: 1000..2000us, neutral 1500us, deadband around center
            uint16_t us = 1500;
            if (abs(eff) >= 5) us = (uint16_t)(1500 + (int32_t)eff * 500 / 100);
            if (us < 1000) us = 1000;
            if (us > 2000) us = 2000;
            if (s_easingSpeedDegS > 0.0f) {
                // Eased µs-space move (EasyServo::write treats value >= 500 as µs)
                auxServo.write((float)us, s_easingSpeedDegS, s_easingKIn, s_easingKOut);
            } else {
                auxServo.writeMicroseconds(us);
            }
            return;
        }

        // H-bridge via EasyMotor: signed percent, direction handled by the driver
        auxMotor.write(eff >= 0 ? (float)pct : -(float)pct);
    }

    // Aux light: config-driven LED output (0..100 percent).
    static void setAuxLight(uint8_t brightnessPct) {
        if (auxLedPin == 0xFF || !auxLed.isAttached()) return;
        auxLed.write((float)brightnessPct);
    }


private:
    static uint8_t s_drivetrainType;
    static MotorChannel s_leftCh;
    static MotorChannel s_rightCh;
    static uint8_t servoPin;

    // Light pins (tracked for setLight routing + hotReload teardown)
    static uint8_t headPin;
    static uint8_t tailPin;
    static uint8_t brakePin;
    static uint8_t turnLPin;
    static uint8_t turnRPin;
    static uint8_t reversingPin;
    static uint8_t ditchLPin;
    static uint8_t ditchRPin;
    static uint8_t stepPin;
    static uint8_t cabPin;


    // Aux motor channel state (mirrors the drive-motor state, but for the aux
    // output). auxMotorType is the electrical kind decided by the hardware
    // token: NONE (not configured / trailer_dcc), DRIVER (H-bridge), ESC (PPM).
    static uint8_t  auxMotorType;        // HardwareConfig::DriveMotor::Type
    static uint8_t  auxMotorDirection;   // HardwareConfig::DriveMotor::Direction
    static uint8_t  auxMotorDutyMin;
    static uint8_t  auxMotorDutyMax;
    static bool     auxMotorAttached;
    static uint8_t  auxLedPin;

    static uint32_t servoFrequency;
    static uint16_t servoLeft;
    static uint16_t servoRight;
    static uint16_t servoCenter;

    // Animation tunables captured from config at init (used by the output paths)
    static float    s_easingSpeedDegS;
    static float    s_easingKIn;
    static float    s_easingKOut;
    static uint16_t s_fadeDurationMs;

    // Blink edge-tracking state (pin -> last commanded active state). A pin is
    // added on first use with active=false so the first active command is a
    // rising edge. Capacity 4 covers both turn pins plus headroom.
    static uint8_t s_blinkPin[4];
    static bool    s_blinkActive[4];

    // EasyKit hardware objects
    static EasyMotor driveMotor;
    static EasyServo steeringServo;
    static EasyServo escServo;      // used when drive motor type == ESC (PPM output)
    static EasyServo auxServo;      // used when aux motor type == ESC (PPM output)
    static EasyMotor auxMotor;      // used when aux motor type == DRIVER (H-bridge)
    static EasyLED   auxLed;        // aux_light LED output
    static EasyLED   headLed;
    static EasyLED   tailLed;
    static EasyLED   brakeLed;
    static EasyLED   turnLLed;
    static EasyLED   turnRLed;
    static EasyLED   reversingLed;
    static EasyLED   ditchLLed;
    static EasyLED   ditchRLed;
    static EasyLED   stepLed;
    static EasyLED   cabLed;
    static EasyLEDGroup s_ditchGroup;
    static bool      s_ditchActive;


    // Config-driven aux outputs. The hardware token decides the channel kind:
    // DRIVER_* → EasyMotor H-bridge, S* → EasyServo PPM, L* → EasyLED. No aux
    // key in the config means no channel is initialized (no legacy auto-attach).
    static void initAuxOutputs(const HardwareConfig& hw) {
        const HardwareConfig::AuxMotor& aux = hw.auxMotor;

        if (aux.motor.type == HardwareConfig::DriveMotor::NONE) {
            auxMotorType = HardwareConfig::DriveMotor::NONE;
            auxMotorAttached = false;
            Serial.println("[HardwareInit] No aux motor configured");
        } else {
            auxMotorType = aux.motor.type;
            auxMotorDirection = aux.motor.direction;
            auxMotorDutyMin = aux.motor.duty.min;
            auxMotorDutyMax = aux.motor.duty.max;

            if (aux.motor.type == HardwareConfig::DriveMotor::DRIVER) {
                const char* name = (aux.motor.hardwareId == PinMapper::DRIVER_A) ? "DRIVER_A" : "DRIVER_B";
                DriverPins pins = PinMapper::getDriver(name);

                if (pins.dualPwm) {
                    // Dual-PWM driver (DRIVER_A): both pins are PWM
                    auxMotor.begin(EasyMotor::DriverType::DRIVER_2PWM,
                                   pins.pwm1, pins.pwm2, pins.enable, false);
                } else {
                    // DIR + PWM driver (DRIVER_B): pin1 = speed PWM, pin2 = direction.
                    // Same polarity convention as the drive motor: invert to preserve.
                    auxMotor.begin(EasyMotor::DriverType::DRIVER_1PWM_1DIR,
                                   pins.pwm1, pins.pwm2, pins.enable, true);
                }
                auxMotor.setFrequency(aux.motor.frequency);
                auxMotorAttached = true;
                Serial.printf("[HardwareInit] Aux motor (driver): %s Freq=%dHz\n",
                              name, aux.motor.frequency);
            }
            else if (aux.motor.type == HardwareConfig::DriveMotor::ESC) {
                EasyKit::ServoConfig cfg;
                cfg.minUs = 1000;
                cfg.maxUs = 2000;
                cfg.centerUs = 1500;
                cfg.freq = (aux.motor.frequency >= 40 && aux.motor.frequency <= 900)
                               ? (uint16_t)aux.motor.frequency : 50;
                if (auxServo.attach(aux.motor.hardwareId, cfg) == EasyKit::Result::OK) {
                    auxMotorAttached = true;
                    Serial.printf("[HardwareInit] Aux motor (servo/ESC): Pin=%d Freq=%dHz\n",
                                  aux.motor.hardwareId, cfg.freq);
                } else {
                    auxMotorAttached = false;
                    Serial.printf("[HardwareInit] Aux motor ESC attach FAILED on Pin=%d\n",
                                  aux.motor.hardwareId);
                }
            }
        }

        auxLedPin = 0xFF;
        if (hw.auxLight.configured) {
            auxLedPin = hw.auxLight.pin;
            const EasyKit::LEDConfig cfg = {5000, EasyKit::LEDCResolution::Bits10, -1, false};
            auxLed.begin(auxLedPin, cfg);
            Serial.printf("[HardwareInit] Aux light: Pin=%d Brightness=%d%%\n",
                          auxLedPin, hw.auxLight.brightness);
        }
    }

    static EasyLED* findLight(uint8_t pin) {
        if (pin == headPin     && headLed.isAttached())     return &headLed;
        if (pin == tailPin     && tailLed.isAttached())     return &tailLed;
        if (pin == brakePin    && brakeLed.isAttached())    return &brakeLed;
        if (pin == turnLPin    && turnLLed.isAttached())    return &turnLLed;
        if (pin == turnRPin    && turnRLed.isAttached())    return &turnRLed;
        if (pin == reversingPin && reversingLed.isAttached()) return &reversingLed;
        if (pin == ditchLPin   && ditchLLed.isAttached())    return &ditchLLed;
        if (pin == ditchRPin   && ditchRLed.isAttached())    return &ditchRLed;
        if (pin == stepPin     && stepLed.isAttached())     return &stepLed;
        if (pin == cabPin      && cabLed.isAttached())      return &cabLed;
        if (pin == auxLedPin   && auxLed.isAttached())      return &auxLed;
        return nullptr;
    }

    static void resetBlinkTracking() {
        for (int i = 0; i < 4; i++) {
            s_blinkPin[i] = 0xFF;
            s_blinkActive[i] = false;
        }
    }

    static int8_t blinkSlot(uint8_t pin) {
        for (int i = 0; i < 4; i++) {
            if (s_blinkPin[i] == pin) return i;
            if (s_blinkPin[i] == 0xFF) {
                s_blinkPin[i] = pin;
                s_blinkActive[i] = false;
                return i;
            }
        }
        return -1;
    }

    // Configure one motor channel from a config: wire the physical output
    // (H-bridge via EasyMotor, or ESC/servo PPM via EasyServo) and capture
    // the polarity/duty state. `driver`/`esc` select which EasyKit object the
    // channel drives — driveMotor/escServo for the drive/left side,
    // auxMotor/auxServo for the skid-steer right side.
    static void initChannel(MotorChannel& ch, const HardwareConfig::DriveMotor& motor,
                            EasyMotor* driver, EasyServo* esc) {
        ch.type = HardwareConfig::DriveMotor::NONE;
        ch.driver = driver;
        ch.esc = esc;
        ch.attached = false;

        if (motor.type == HardwareConfig::DriveMotor::NONE) {
            Serial.println("[HardwareInit] No motor configured");
            return;
        }

        ch.type = motor.type;
        ch.direction = motor.direction;
        ch.dutyMin = motor.duty.min;
        ch.dutyMax = motor.duty.max;
        ch.frequency = motor.frequency;

        if (motor.type == HardwareConfig::DriveMotor::DRIVER) {
            const char* name = (motor.hardwareId == PinMapper::DRIVER_A) ? "DRIVER_A" : "DRIVER_B";
            DriverPins pins = PinMapper::getDriver(name);

            if (pins.dualPwm) {
                // Dual-PWM driver (DRIVER_A): both pins are PWM
                ch.driver->begin(EasyMotor::DriverType::DRIVER_2PWM,
                                 pins.pwm1, pins.pwm2, pins.enable, false);
            } else {
                // DIR + PWM driver (DRIVER_B): pin1 = speed PWM, pin2 = direction.
                // EasyKit drives DIR HIGH for forward; this driver drives DIR LOW for
                // forward, so invert to preserve the previous polarity.
                ch.driver->begin(EasyMotor::DriverType::DRIVER_1PWM_1DIR,
                                 pins.pwm1, pins.pwm2, pins.enable, true);
            }
            ch.driver->setFrequency(ch.frequency);
            ch.attached = true;

            Serial.printf("[HardwareInit] Driver: %s PWM1=%d PWM2=%d EN=%d Freq=%dHz\n",
                          name, pins.pwm1, pins.pwm2, pins.enable, motor.frequency);
        }
        else if (motor.type == HardwareConfig::DriveMotor::ESC && ch.esc) {
            EasyKit::ServoConfig cfg;
            cfg.minUs = 1000;
            cfg.maxUs = 2000;
            cfg.centerUs = 1500;
            // ESC runs on a servo-style PPM pulse; fall back to 50 Hz if the
            // config frequency is not a sane servo refresh rate.
            cfg.freq = (ch.frequency >= 40 && ch.frequency <= 900)
                           ? (uint16_t)ch.frequency : 50;

            if (ch.esc->attach(motor.hardwareId, cfg) == EasyKit::Result::OK) {
                ch.attached = true;
                Serial.printf("[HardwareInit] ESC: Pin=%d Freq=%dHz\n", motor.hardwareId, cfg.freq);
            } else {
                Serial.printf("[HardwareInit] ESC attach FAILED on Pin=%d\n", motor.hardwareId);
            }
        }
    }

    static void initSteeringServo(const HardwareConfig::SteeringServo& servo) {
        if (servo.hardwareId == 0) {
            servoPin = 0xFF;
            Serial.println("[HardwareInit] No steering servo configured");
            return;
        }

        servoPin = servo.hardwareId;
        servoFrequency = servo.frequency;
        servoLeft = servo.endpoints.left;
        servoRight = servo.endpoints.right;
        servoCenter = servo.endpoints.center;
        s_lastServoPos = -999;

        // Attach with full range (500..2500us) so writeMicroseconds is never truncated or double-scaled
        EasyKit::ServoConfig cfg;
        cfg.minUs = 500;
        cfg.maxUs = 2500;
        cfg.centerUs = servoCenter;
        cfg.freq = (servoFrequency >= 40 && servoFrequency <= 400) ? servoFrequency : 50;

        if (steeringServo.attach(servoPin, cfg) == EasyKit::Result::OK) {
            Serial.printf("[HardwareInit] Servo: Pin=%d Freq=%dHz Center=%dus (L=%d, R=%d)\n",
                          servoPin, cfg.freq, servoCenter, servoLeft, servoRight);
            steeringServo.writeMicroseconds(servoCenter);
        } else {
            Serial.printf("[HardwareInit] Servo attach FAILED on Pin=%d\n", servoPin);
        }
    }

    static void initLights(const HardwareConfig::Lights& lights) {
        headPin = 0xFF; tailPin = 0xFF; brakePin = 0xFF;
        turnLPin = 0xFF; turnRPin = 0xFF; reversingPin = 0xFF;
        ditchLPin = 0xFF; ditchRPin = 0xFF; stepPin = 0xFF; cabPin = 0xFF;

        const EasyKit::LEDConfig cfg = {5000, EasyKit::LEDCResolution::Bits10, -1, false};

        if (lights.headLight.configured) {
            headPin = lights.headLight.pin;
            headLed.begin(headPin, cfg);
            Serial.printf("[HardwareInit] Headlight: Pin=%d Brightness=%d%%\n",
                          headPin, lights.headLight.brightness);
        }

        if (lights.tailLight.configured) {
            tailPin = lights.tailLight.pin;
            tailLed.begin(tailPin, cfg);
            Serial.printf("[HardwareInit] Taillight: Pin=%d Brightness=%d%%\n",
                          tailPin, lights.tailLight.brightness);
        }

        if (lights.brakeLight.configured) {
            brakePin = lights.brakeLight.pin;
            brakeLed.begin(brakePin, cfg);
            Serial.printf("[HardwareInit] Brakelight: Pin=%d\n", brakePin);
        }

        if (lights.ditchLight.configured) {
            ditchLPin = lights.ditchLight.leftPin;
            ditchRPin = lights.ditchLight.rightPin;
            if (ditchLPin != 0xFF) ditchLLed.begin(ditchLPin, cfg);
            if (ditchRPin != 0xFF) ditchRLed.begin(ditchRPin, cfg);
            s_ditchGroup.clearMembers();
            if (ditchLPin != 0xFF) s_ditchGroup.addMember(&ditchLLed);
            if (ditchRPin != 0xFF) s_ditchGroup.addMember(&ditchRLed);
            s_ditchActive = false;
            Serial.printf("[HardwareInit] Ditch lights: L=%d R=%d Interval=%dms Brightness=%d%%\n",
                          ditchLPin, ditchRPin, lights.ditchLight.intervalMs,
                          lights.ditchLight.brightness);
        }


        if (lights.stepLight.configured) {
            stepPin = lights.stepLight.pin;
            stepLed.begin(stepPin, cfg);
            Serial.printf("[HardwareInit] Step light: Pin=%d Brightness=%d%%\n",
                          stepPin, lights.stepLight.brightness);
        }

        if (lights.cabLight.configured) {
            cabPin = lights.cabLight.pin;
            cabLed.begin(cabPin, cfg);
            Serial.printf("[HardwareInit] Cab light: Pin=%d Brightness=%d%%\n",
                          cabPin, lights.cabLight.brightness);
        }

        if (lights.turnLight.configured) {
            turnLPin = lights.turnLight.leftPin;
            turnRPin = lights.turnLight.rightPin;
            if (turnLPin != 0xFF) turnLLed.begin(turnLPin, cfg);
            if (turnRPin != 0xFF) turnRLed.begin(turnRPin, cfg);
            Serial.printf("[HardwareInit] Turn signals: L=%d R=%d Interval=%d/%dms\n",
                          turnLPin, turnRPin,
                          lights.turnLight.intervalOn, lights.turnLight.intervalOff);
        }

        if (lights.reversingLight.configured) {
            reversingPin = lights.reversingLight.pin;
            // May alias another light's pin (e.g. BRAKE_LIGHT) — only attach a
            // second LED object if this is a distinct physical output. setLight()
            // routes by pin value, so the shared pin drives the existing LED.
            if (reversingPin != headPin && reversingPin != tailPin &&
                reversingPin != brakePin && reversingPin != turnLPin &&
                reversingPin != turnRPin) {
                reversingLed.begin(reversingPin, cfg);
            }
            Serial.printf("[HardwareInit] Reversing light: Pin=%d%s\n", reversingPin,
                          reversingPin == brakePin ? " (shares brake output)" : "");
        }
    }

private:
    static bool     s_powerLatched;
    static bool     s_powerButtonHolding;
    static uint32_t s_powerButtonHoldStart;
    static bool     s_buttonClicked;
};

bool     HardwareInit::s_powerLatched = false;
bool     HardwareInit::s_powerButtonHolding = false;
uint32_t HardwareInit::s_powerButtonHoldStart = 0;
bool     HardwareInit::s_buttonClicked = false;

uint8_t HardwareInit::servoPin = 0xFF;

HardwareInit::MotorChannel HardwareInit::s_leftCh;
HardwareInit::MotorChannel HardwareInit::s_rightCh;

uint8_t HardwareInit::headPin = 0xFF;
uint8_t HardwareInit::tailPin = 0xFF;
uint8_t HardwareInit::brakePin = 0xFF;
uint8_t HardwareInit::turnLPin = 0xFF;
uint8_t HardwareInit::turnRPin = 0xFF;
uint8_t HardwareInit::reversingPin = 0xFF;
uint8_t HardwareInit::ditchLPin = 0xFF;
uint8_t HardwareInit::ditchRPin = 0xFF;
uint8_t HardwareInit::stepPin = 0xFF;
uint8_t HardwareInit::cabPin = 0xFF;


uint8_t  HardwareInit::auxMotorType = HardwareConfig::DriveMotor::NONE;
uint8_t  HardwareInit::auxMotorDirection = HardwareConfig::DriveMotor::FORWARD;
uint8_t  HardwareInit::auxMotorDutyMin = 20;
uint8_t  HardwareInit::auxMotorDutyMax = 90;
bool     HardwareInit::auxMotorAttached = false;
uint8_t  HardwareInit::auxLedPin = 0xFF;

uint32_t HardwareInit::servoFrequency = 50;
uint16_t HardwareInit::servoLeft = 1350;
uint16_t HardwareInit::servoRight = 1650;
uint16_t HardwareInit::servoCenter = 1500;

float    HardwareInit::s_easingSpeedDegS = 180.0f;
float    HardwareInit::s_easingKIn = 0.2f;
float    HardwareInit::s_easingKOut = 0.8f;
uint16_t HardwareInit::s_fadeDurationMs = 250;

uint8_t  HardwareInit::s_blinkPin[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
bool     HardwareInit::s_blinkActive[4] = { false, false, false, false };

EasyMotor HardwareInit::driveMotor;
EasyServo HardwareInit::steeringServo;
EasyServo HardwareInit::escServo;
EasyServo HardwareInit::auxServo;
EasyMotor HardwareInit::auxMotor;
EasyLED   HardwareInit::auxLed;
EasyLED   HardwareInit::headLed;
EasyLED   HardwareInit::tailLed;
EasyLED   HardwareInit::brakeLed;
EasyLED   HardwareInit::turnLLed;
EasyLED   HardwareInit::turnRLed;
EasyLED   HardwareInit::reversingLed;
EasyLED   HardwareInit::ditchLLed;
EasyLED   HardwareInit::ditchRLed;
EasyLED   HardwareInit::stepLed;
EasyLED   HardwareInit::cabLed;
EasyLEDGroup HardwareInit::s_ditchGroup;
bool      HardwareInit::s_ditchActive = false;
uint8_t   HardwareInit::s_drivetrainType = HardwareConfig::ACKERMANN;
int16_t   HardwareInit::s_lastServoPos = -999;

