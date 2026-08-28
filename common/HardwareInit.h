#pragma once

#include <Arduino.h>
#include "Config.h"
#include "PinMapper.h"
#include <EasyMotor.h>
#include <EasyServo.h>
#include <EasyLED.h>
#include <EasyLEDGroup.h>

class HardwareInit {

    struct MotorChannel {
        uint8_t    type = HardwareConfig::DriveMotor::NONE;   // DRIVER | ESC
        uint8_t    direction = HardwareConfig::DriveMotor::FORWARD;
        uint8_t    dutyMin = 20;
        uint8_t    dutyMax = 90;
        uint32_t   frequency = 20000;
        uint8_t    bemfPin = 0xFF;
        bool       attached = false;
        EasyMotor* driver = nullptr;   // used when type == DRIVER
        EasyServo* esc = nullptr;      // used when type == ESC
    };

public:
    static void init(const HardwareConfig& hw) {
        Serial.println("[HardwareInit] Initializing peripherals...");

        s_easingSpeedDegS = hw.animation.easingSpeedDegS;
        s_easingKIn       = hw.animation.easingKIn;
        s_easingKOut      = hw.animation.easingKOut;
        s_fadeDurationMs  = hw.animation.fadeDurationMs;
        resetBlinkTracking();

        for (uint8_t i = 0; i < HardwareConfig::MAX_DRIVE_MOTORS; i++) {
            s_driveCh[i] = MotorChannel();
        }
        s_driveChCount = 0;
        s_rightCh = MotorChannel();
        s_primaryBemfPin = 0xFF;
        s_drivetrainType = hw.drivetrainType;

        if (hw.drivetrainType == HardwareConfig::SKID_STEER) {
            initChannel(s_driveCh[0], hw.leftMotor, &driveMotor, &escServo);
            initChannel(s_rightCh, hw.rightMotor, &auxMotor, &auxServo);
            s_driveChCount = 1;
            s_primaryBemfPin = s_driveCh[0].bemfPin;
        } else {
            s_driveChCount = 0;
            uint8_t effDriveCount = hw.driveMotorCount;
            if (effDriveCount == 0 && hw.driveMotors[0].type != HardwareConfig::DriveMotor::NONE) {
                effDriveCount = 1;
            }
            for (uint8_t i = 0; i < effDriveCount && i < HardwareConfig::MAX_DRIVE_MOTORS; i++) {
                EasyMotor* drv = (hw.driveMotors[i].hardwareId == PinMapper::DRIVER_B) ? &auxMotor : &driveMotor;
                EasyServo* esc = (i == 0) ? &escServo : &auxServo;
                initChannel(s_driveCh[i], hw.driveMotors[i], drv, esc);
                if (s_driveCh[i].attached) {
                    s_driveChCount++;
                }
            }
            if (s_driveChCount > 0) {
                s_primaryBemfPin = s_driveCh[0].bemfPin;
            }
            initSteeringServos(hw);
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
        for (uint8_t i = 0; i < HardwareConfig::MAX_STEERING_SERVOS; i++) {
            steeringServos[i].stop();
            steeringServos[i].detach();
        }
        escServo.stop();
        escServo.detach();
        auxServo.stop();
        auxServo.detach();
        stopLightAnimations();

        driveMotor.end();
        auxMotor.end();

        for (uint8_t p = 0; p < headPinCount; p++) headLeds[p].end();
        for (uint8_t p = 0; p < fullPinCount; p++) fullLeds[p].end();
        for (uint8_t p = 0; p < fogPinCount; p++) fogLeds[p].end();
        for (uint8_t p = 0; p < tailPinCount; p++) tailLeds[p].end();
        for (uint8_t p = 0; p < brakePinCount; p++) brakeLeds[p].end();
        for (uint8_t p = 0; p < reversingPinCount; p++) reversingLeds[p].end();
        for (uint8_t p = 0; p < beaconPinCount; p++) beaconLeds[p].end();
        for (uint8_t p = 0; p < cabPinCount; p++) cabLeds[p].end();
        for (uint8_t p = 0; p < workPinCount; p++) workLeds[p].end();
        for (uint8_t p = 0; p < stepPinCount; p++) stepLeds[p].end();
        for (uint8_t p = 0; p < auxPinCount; p++) auxLeds[p].end();
        turnLLed.end();
        turnRLed.end();
        ditchLLed.end();
        ditchRLed.end();

        for (uint8_t i = 0; i < HardwareConfig::MAX_DRIVE_MOTORS; i++) {
            s_driveCh[i].attached = false;
        }
        s_rightCh.attached = false;
        Serial.println("[HardwareInit] Stopped all outputs");
    }

    static void update(uint16_t buttonHoldS = 4, uint8_t indicatorPin = 0xFF) {
        updatePowerButton(buttonHoldS, indicatorPin);
        for (uint8_t i = 0; i < s_steeringServoCount; i++) {
            steeringServos[i].update();
        }
        escServo.update();
        auxServo.update();

        for (uint8_t p = 0; p < headPinCount; p++) headLeds[p].update();
        for (uint8_t p = 0; p < fullPinCount; p++) fullLeds[p].update();
        for (uint8_t p = 0; p < fogPinCount; p++) fogLeds[p].update();
        for (uint8_t p = 0; p < tailPinCount; p++) tailLeds[p].update();
        for (uint8_t p = 0; p < brakePinCount; p++) brakeLeds[p].update();
        for (uint8_t p = 0; p < reversingPinCount; p++) reversingLeds[p].update();
        for (uint8_t p = 0; p < beaconPinCount; p++) beaconLeds[p].update();
        for (uint8_t p = 0; p < cabPinCount; p++) cabLeds[p].update();
        for (uint8_t p = 0; p < workPinCount; p++) workLeds[p].update();
        for (uint8_t p = 0; p < stepPinCount; p++) stepLeds[p].update();
        for (uint8_t p = 0; p < auxPinCount; p++) auxLeds[p].update();

        turnLLed.update();
        turnRLed.update();
        ditchLLed.update();
        ditchRLed.update();
        s_ditchGroup.update();
    }

    static void latchPower(uint16_t bootLatchS = 1) {
        if (POWER::POWER_ENABLE == 0xFF || POWER::POWER_BUTTON == 0xFF) {
            Serial.println("[HardwareInit] Board lacks power control pins; skipping latchPower()");
            s_powerLatched = true;
            return;
        }
        pinMode(POWER::POWER_ENABLE, OUTPUT);
        digitalWrite(POWER::POWER_ENABLE, LOW);
        pinMode(POWER::POWER_BUTTON, INPUT_PULLDOWN);
        if (POWER::CHARGE_SENS != 0xFF) {
            pinMode(POWER::CHARGE_SENS, INPUT);
        }
        s_powerLatched = false;
        s_powerButtonHolding = false;
        s_powerButtonHoldStart = 0;
        s_buttonClicked = false;

        if (digitalRead(POWER::POWER_BUTTON) == LOW) {
            digitalWrite(POWER::POWER_ENABLE, HIGH);
            s_powerLatched = true;
            Serial.println("[HardwareInit] Power latched via USB boot");
            return;
        }

        uint32_t pressStart = millis();
        uint32_t targetMs = (uint32_t)bootLatchS * 1000U;
        Serial.printf("[HardwareInit] Waiting for power button hold (%ds)...\n", bootLatchS);
        while (digitalRead(POWER::POWER_BUTTON) == HIGH) {
            if (millis() - pressStart >= targetMs) {
                digitalWrite(POWER::POWER_ENABLE, HIGH);
                s_powerLatched = true;
                s_powerButtonHolding = false;
                s_powerButtonHoldStart = 0;
                Serial.println("[HardwareInit] Power successfully latched ON");
                break;
            }
            delay(10);
        }

        if (!s_powerLatched) {
            Serial.println("[HardwareInit] Power button released too early; shutting down");
            digitalWrite(POWER::POWER_ENABLE, LOW);
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

            // Rapid blink feedback (200ms ON / 200ms OFF) only after confirmed 500ms hold
            if (now - s_powerButtonHoldStart >= 500) {
                uint8_t duty = ((now / 200) % 2 == 0) ? 100 : 0;
                if (indicatorPin != 0xFF) {
                    setLight(indicatorPin, duty);
                } else {
                    setLight(turnLPin, duty);
                    setLight(turnRPin, duty);
                }
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
                uint32_t duration = now - s_powerButtonHoldStart;
                s_powerButtonHolding = false;
                s_powerButtonHoldStart = 0;

                // Single click: 50ms <= duration < 500ms
                if (duration >= 50 && duration < 500) {
                    s_buttonClicked = true;
                    Serial.println("[HardwareInit] Power button single-click detected");
                }
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
        return digitalRead(POWER::CHARGE_SENS) == LOW;
    }

    static void powerOff() {
        Serial.println("[HardwareInit] Powering off hardware...");
        stopAll();
        if (POWER::POWER_ENABLE != 0xFF) {
            digitalWrite(POWER::POWER_ENABLE, LOW);
        }
        s_powerLatched = false;
        s_powerButtonHolding = false;
        s_powerButtonHoldStart = 0;
        s_buttonClicked = false;
    }

    static bool isPowerLatched() { return s_powerLatched; }

    static void setSkidMotors(int16_t leftSpeed, int16_t rightSpeed) {
        setChannel(s_driveCh[0], leftSpeed);
        setChannel(s_rightCh, rightSpeed);
    }

    static void setAllMotors(int16_t speed) {
        for (uint8_t i = 0; i < s_driveChCount; i++) {
            setChannel(s_driveCh[i], speed);
        }
        setChannel(s_rightCh, speed);
    }

    static void setMotor(int16_t speed) {
        for (uint8_t i = 0; i < s_driveChCount; i++) {
            setChannel(s_driveCh[i], speed);
        }
    }

    static uint8_t getPrimaryBemfPin() {
        return s_primaryBemfPin;
    }

    static void setChannel(MotorChannel& ch, int16_t speed) {
        if (ch.type == HardwareConfig::DriveMotor::NONE || !ch.attached) return;

        int16_t eff = speed;
        switch (ch.direction) {
            case HardwareConfig::DriveMotor::REVERSE:     eff = -eff;      break;
            case HardwareConfig::DriveMotor::UNI_FORWARD: eff = abs(eff);  break;
            case HardwareConfig::DriveMotor::UNI_REVERSE: eff = -abs(eff); break;
            default: break;
        }

        uint8_t pct = 0;
        if (eff > 0) {
            pct = ch.dutyMin + (uint8_t)((uint32_t)eff * (ch.dutyMax - ch.dutyMin) / 100);
        } else if (eff < 0) {
            pct = ch.dutyMin + (uint8_t)((uint32_t)(-eff) * (ch.dutyMax - ch.dutyMin) / 100);
        }
        if (pct > ch.dutyMax) pct = ch.dutyMax;

        if (ch.type == HardwareConfig::DriveMotor::ESC) {
            uint16_t us = 1500;
            if (abs(eff) >= 5) us = (uint16_t)(1500 + (int32_t)eff * 500 / 100);
            if (us < 1000) us = 1000;
            if (us > 2000) us = 2000;
            if (ch.esc) ch.esc->writeMicroseconds(us);
            return;
        }

        if (ch.driver) ch.driver->write(eff >= 0 ? (float)pct : -(float)pct);
    }

    static int16_t s_lastServoPos;
    static void setServo(int16_t position) {
        if (s_steeringServoCount == 0) return;
        if (position == s_lastServoPos) return;
        s_lastServoPos = position;

        for (uint8_t i = 0; i < s_steeringServoCount; i++) {
            if (!steeringServos[i].attached()) continue;
            const auto& ep = s_steeringConfigs[i].endpoints;
            int32_t us = ep.center;
            if (position > 0)      us = ep.center + (int32_t)position * (ep.right - ep.center) / 100;
            else if (position < 0) us = ep.center + (int32_t)position * (ep.center - ep.left) / 100;
            if (us < 500)  us = 500;
            if (us > 2500) us = 2500;

            steeringServos[i].writeMicroseconds(us);
        }
    }

    static void setLight(uint8_t pin, uint8_t brightnessPct) {
        if (pin == 0xFF || pin == 0) return;
        EasyLED* led = findLight(pin);
        if (led) {
            led->write((float)brightnessPct);
        } else {
            pinMode(pin, OUTPUT);
            digitalWrite(pin, brightnessPct > 0 ? HIGH : LOW);
        }
    }

    static void setLight(const HardwareConfig::Lights::Light& light, uint8_t brightnessPct) {
        if (!light.configured) return;
        for (uint8_t i = 0; i < light.pinCount; i++) {
            setLight(light.pins[i], brightnessPct);
        }
    }

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

    static void setLightFade(uint8_t pin, uint8_t targetPct, uint16_t durationMs) {
        if (pin == 0xFF || pin == 0) return;
        EasyLED* led = findLight(pin);
        if (led) {
            uint32_t targetTicks = (uint32_t)led->getMaxDuty() * targetPct / 100;
            led->fadeTo(targetTicks, durationMs, EasyLED::Curve::EASE_IN_OUT);
        } else {
            pinMode(pin, OUTPUT);
            digitalWrite(pin, targetPct > 0 ? HIGH : LOW);
        }
    }

    static void setLightFade(const HardwareConfig::Lights::Light& light, uint8_t targetPct, uint16_t durationMs) {
        if (!light.configured) return;
        for (uint8_t i = 0; i < light.pinCount; i++) {
            setLightFade(light.pins[i], targetPct, durationMs);
        }
    }

    static uint8_t getLightDutyPercent(uint8_t pin) {
        if (pin == 0xFF || pin == 0) return 0;
        EasyLED* led = findLight(pin);
        if (!led) return 0;
        return (uint8_t)led->getDutyPercent();
    }

    static void stopLightAnimations() {
        for (uint8_t p = 0; p < headPinCount; p++) headLeds[p].stop();
        for (uint8_t p = 0; p < fullPinCount; p++) fullLeds[p].stop();
        for (uint8_t p = 0; p < fogPinCount; p++) fogLeds[p].stop();
        for (uint8_t p = 0; p < tailPinCount; p++) tailLeds[p].stop();
        for (uint8_t p = 0; p < brakePinCount; p++) brakeLeds[p].stop();
        for (uint8_t p = 0; p < reversingPinCount; p++) reversingLeds[p].stop();
        for (uint8_t p = 0; p < beaconPinCount; p++) beaconLeds[p].stop();
        for (uint8_t p = 0; p < cabPinCount; p++) cabLeds[p].stop();
        for (uint8_t p = 0; p < workPinCount; p++) workLeds[p].stop();
        for (uint8_t p = 0; p < stepPinCount; p++) stepLeds[p].stop();
        for (uint8_t p = 0; p < auxPinCount; p++) auxLeds[p].stop();
        turnLLed.stop();
        turnRLed.stop();
        ditchLLed.stop();
        ditchRLed.stop();
        s_ditchGroup.stop();
        s_ditchActive = false;
        resetBlinkTracking();
    }

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

    static void setAuxMotor(int16_t speed) {
        if (s_auxMotorCount == 0) return;
        for (uint8_t i = 0; i < s_auxMotorCount; i++) {
            const auto& aux = s_auxConfigs[i];
            if (aux.purpose == HardwareConfig::AuxMotor::NONE) continue;
            int16_t eff = speed;
            switch (aux.motor.direction) {
                case HardwareConfig::DriveMotor::REVERSE:     eff = -eff;      break;
                case HardwareConfig::DriveMotor::UNI_FORWARD: eff = abs(eff);  break;
                case HardwareConfig::DriveMotor::UNI_REVERSE: eff = -abs(eff); break;
                default: break;
            }

            uint8_t pct = 0;
            if (eff > 0) {
                pct = aux.motor.duty.min + (uint8_t)((uint32_t)eff * (aux.motor.duty.max - aux.motor.duty.min) / 100);
            } else if (eff < 0) {
                pct = aux.motor.duty.min + (uint8_t)((uint32_t)(-eff) * (aux.motor.duty.max - aux.motor.duty.min) / 100);
            }
            if (pct > aux.motor.duty.max) pct = aux.motor.duty.max;

            if (aux.motor.type == HardwareConfig::DriveMotor::ESC) {
                uint16_t us = 1500;
                if (abs(eff) >= 5) us = (uint16_t)(1500 + (int32_t)eff * 500 / 100);
                if (us < 1000) us = 1000;
                if (us > 2000) us = 2000;
                auxServo.writeMicroseconds(us);
            } else if (aux.motor.type == HardwareConfig::DriveMotor::DRIVER) {
                auxMotor.write(eff >= 0 ? (float)pct : -(float)pct);
            }
        }
    }

    static void setAuxLight(uint8_t brightnessPct) {
        for (uint8_t p = 0; p < auxPinCount; p++) {
            setLight(auxPins[p], brightnessPct);
        }
    }

    static void setBeacon(bool active, uint16_t intervalMs = 120) {
        for (uint8_t p = 0; p < beaconPinCount; p++) {
            setLightBlink(beaconPins[p], active, intervalMs, intervalMs, 100);
        }
    }

    static uint8_t getConfiguredLightMask(const HardwareConfig::Lights& L, bool isLoco) {
        uint8_t mask = 0;
        if (!isLoco) {
            if (L.headLight.configured) mask |= (1 << 0);
            if (L.fullBeam.configured || L.headLight.configured) mask |= (1 << 1);
            if (L.fogLamp.configured) mask |= (1 << 2);
            if (L.turnLight.configured) mask |= (1 << 3);
            if (L.beacon.configured) mask |= (1 << 4);
            if (L.cabLight.configured) mask |= (1 << 5);
            if (L.workLight.configured) mask |= (1 << 6);
            if (L.auxLight.configured || auxPinCount > 0) mask |= (1 << 7);
        } else {
            if (L.headLight.configured || L.tailLight.configured) mask |= (1 << 0);
            if (L.ditchLight.configured) mask |= (1 << 1);
            if (L.cabLight.configured) mask |= (1 << 2);
            if (L.stepLight.configured) mask |= (1 << 3);
            if (L.beacon.configured || L.fogLamp.configured || L.fullBeam.configured) mask |= (1 << 4);
            if (L.auxLight.configured || L.workLight.configured || auxPinCount > 0) mask |= (1 << 5);
        }
        return mask;
    }

    static uint8_t s_drivetrainType;
    static MotorChannel s_driveCh[HardwareConfig::MAX_DRIVE_MOTORS];
    static uint8_t      s_driveChCount;
    static MotorChannel s_rightCh;
    static uint8_t      s_primaryBemfPin;

    static uint8_t headPins[HardwareConfig::MAX_PINS_PER_LIGHT];
    static uint8_t headPinCount;
    static uint8_t fullPins[HardwareConfig::MAX_PINS_PER_LIGHT];
    static uint8_t fullPinCount;
    static uint8_t fogPins[HardwareConfig::MAX_PINS_PER_LIGHT];
    static uint8_t fogPinCount;
    static uint8_t tailPins[HardwareConfig::MAX_PINS_PER_LIGHT];
    static uint8_t tailPinCount;
    static uint8_t brakePins[HardwareConfig::MAX_PINS_PER_LIGHT];
    static uint8_t brakePinCount;
    static uint8_t turnLPin;
    static uint8_t turnRPin;
    static uint8_t reversingPins[HardwareConfig::MAX_PINS_PER_LIGHT];
    static uint8_t reversingPinCount;
    static uint8_t beaconPins[HardwareConfig::MAX_PINS_PER_LIGHT];
    static uint8_t beaconPinCount;
    static uint8_t cabPins[HardwareConfig::MAX_PINS_PER_LIGHT];
    static uint8_t cabPinCount;
    static uint8_t workPins[HardwareConfig::MAX_PINS_PER_LIGHT];
    static uint8_t workPinCount;
    static uint8_t stepPins[HardwareConfig::MAX_PINS_PER_LIGHT];
    static uint8_t stepPinCount;
    static uint8_t ditchLPin;
    static uint8_t ditchRPin;
    static uint8_t auxPins[HardwareConfig::MAX_PINS_PER_LIGHT];
    static uint8_t auxPinCount;

    static HardwareConfig::AuxMotor s_auxConfigs[HardwareConfig::MAX_AUX_MOTORS];
    static uint8_t                  s_auxMotorCount;

    static HardwareConfig::SteeringServo s_steeringConfigs[HardwareConfig::MAX_STEERING_SERVOS];
    static uint8_t                       s_steeringServoCount;

    static float    s_easingSpeedDegS;
    static float    s_easingKIn;
    static float    s_easingKOut;
    static uint16_t s_fadeDurationMs;
    static uint8_t  s_ditchBrightness;

    static uint8_t s_blinkPin[4];
    static bool    s_blinkActive[4];

    static EasyMotor driveMotor;
    static EasyServo steeringServos[HardwareConfig::MAX_STEERING_SERVOS];
    static EasyServo escServo;
    static EasyServo auxServo;
    static EasyMotor auxMotor;

    static EasyLED   headLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
    static EasyLED   fullLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
    static EasyLED   fogLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
    static EasyLED   tailLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
    static EasyLED   brakeLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
    static EasyLED   reversingLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
    static EasyLED   beaconLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
    static EasyLED   cabLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
    static EasyLED   workLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
    static EasyLED   stepLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
    static EasyLED   auxLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
    static EasyLED   turnLLed;
    static EasyLED   turnRLed;
    static EasyLED   ditchLLed;
    static EasyLED   ditchRLed;
    static EasyLEDGroup s_ditchGroup;
    static bool      s_ditchActive;

    static void initAuxOutputs(const HardwareConfig& hw) {
        if (hw.drivetrainType == HardwareConfig::SKID_STEER) {
            s_auxMotorCount = 0;
            auxPinCount = 0;
            return;
        }
        uint8_t effAuxCount = hw.auxMotorCount;
        if (effAuxCount == 0 && hw.auxMotors[0].configured) {
            effAuxCount = 1;
        }
        s_auxMotorCount = effAuxCount;
        for (uint8_t i = 0; i < effAuxCount && i < HardwareConfig::MAX_AUX_MOTORS; i++) {
            s_auxConfigs[i] = hw.auxMotors[i];
            const auto& aux = hw.auxMotors[i];
            if (aux.motor.type == HardwareConfig::DriveMotor::DRIVER) {
                const char* name = (aux.motor.hardwareId == PinMapper::DRIVER_A) ? "DRIVER_A" : "DRIVER_B";
                DriverPins pins = PinMapper::getDriver(name);
                if (pins.dualPwm) {
                    auxMotor.begin(EasyMotor::DriverType::DRIVER_2PWM,
                                   pins.pwm1, pins.pwm2, pins.enable, false);
                } else {
                    auxMotor.begin(EasyMotor::DriverType::DRIVER_1PWM_1DIR,
                                   pins.pwm1, pins.pwm2, pins.enable, true);
                }
                auxMotor.setFrequency(aux.motor.frequency);
            } else if (aux.motor.type == HardwareConfig::DriveMotor::ESC) {
                EasyKit::ServoConfig cfg;
                cfg.minUs = 1000;
                cfg.maxUs = 2000;
                cfg.centerUs = 1500;
                cfg.freq = (aux.motor.frequency >= 40 && aux.motor.frequency <= 900)
                               ? (uint16_t)aux.motor.frequency : 50;
                auxServo.attach(aux.motor.hardwareId, cfg);
            }
        }

        if (hw.auxLight.configured) {
            auxPinCount = 0;
            uint8_t effectiveCount = hw.auxLight.pinCount;
            if (effectiveCount == 0 && hw.auxLight.pin != 0xFF) effectiveCount = 1;
            const EasyKit::LEDConfig cfg = {5000, EasyKit::LEDCResolution::Bits10, -1, false};
            for (uint8_t p = 0; p < effectiveCount && p < HardwareConfig::MAX_PINS_PER_LIGHT; p++) {
                auxPins[p] = (p < hw.auxLight.pinCount) ? hw.auxLight.pins[p] : hw.auxLight.pin;
                auxLeds[p].begin(auxPins[p], cfg);
                auxPinCount++;
            }
        }
    }

    static EasyLED* findLight(uint8_t pin) {
        for (uint8_t p = 0; p < headPinCount; p++) if (pin == headPins[p] && headLeds[p].isAttached()) return &headLeds[p];
        for (uint8_t p = 0; p < fullPinCount; p++) if (pin == fullPins[p] && fullLeds[p].isAttached()) return &fullLeds[p];
        for (uint8_t p = 0; p < fogPinCount; p++) if (pin == fogPins[p] && fogLeds[p].isAttached()) return &fogLeds[p];
        for (uint8_t p = 0; p < tailPinCount; p++) if (pin == tailPins[p] && tailLeds[p].isAttached()) return &tailLeds[p];
        for (uint8_t p = 0; p < brakePinCount; p++) if (pin == brakePins[p] && brakeLeds[p].isAttached()) return &brakeLeds[p];
        for (uint8_t p = 0; p < reversingPinCount; p++) if (pin == reversingPins[p] && reversingLeds[p].isAttached()) return &reversingLeds[p];
        for (uint8_t p = 0; p < beaconPinCount; p++) if (pin == beaconPins[p] && beaconLeds[p].isAttached()) return &beaconLeds[p];
        for (uint8_t p = 0; p < cabPinCount; p++) if (pin == cabPins[p] && cabLeds[p].isAttached()) return &cabLeds[p];
        for (uint8_t p = 0; p < workPinCount; p++) if (pin == workPins[p] && workLeds[p].isAttached()) return &workLeds[p];
        for (uint8_t p = 0; p < stepPinCount; p++) if (pin == stepPins[p] && stepLeds[p].isAttached()) return &stepLeds[p];
        for (uint8_t p = 0; p < auxPinCount; p++) if (pin == auxPins[p] && auxLeds[p].isAttached()) return &auxLeds[p];

        if (pin == turnLPin && turnLLed.isAttached()) return &turnLLed;
        if (pin == turnRPin && turnRLed.isAttached()) return &turnRLed;
        if (pin == ditchLPin && ditchLLed.isAttached()) return &ditchLLed;
        if (pin == ditchRPin && ditchRLed.isAttached()) return &ditchRLed;
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

    static void initChannel(MotorChannel& ch, const HardwareConfig::DriveMotor& motor,
                            EasyMotor* driver, EasyServo* esc) {
        if (motor.type == HardwareConfig::DriveMotor::NONE) {
            ch.type = HardwareConfig::DriveMotor::NONE;
            ch.attached = false;
            return;
        }

        ch.type = motor.type;
        ch.direction = motor.direction;
        ch.dutyMin = motor.duty.min;
        ch.dutyMax = motor.duty.max;
        ch.frequency = motor.frequency;
        ch.driver = driver;
        ch.esc = esc;

        if (motor.type == HardwareConfig::DriveMotor::DRIVER) {
            const char* name = (motor.hardwareId == PinMapper::DRIVER_A) ? "DRIVER_A" : "DRIVER_B";
            DriverPins pins = PinMapper::getDriver(name);
            ch.bemfPin = pins.bemf;

            if (pins.dualPwm) {
                driver->begin(EasyMotor::DriverType::DRIVER_2PWM,
                              pins.pwm1, pins.pwm2, pins.enable, false);
            } else {
                driver->begin(EasyMotor::DriverType::DRIVER_1PWM_1DIR,
                              pins.pwm1, pins.pwm2, pins.enable, true);
            }
            driver->setFrequency(motor.frequency);
            ch.attached = true;
            Serial.printf("[HardwareInit] Motor (driver): %s Freq=%dHz Duty=%d..%d%%\n",
                          name, motor.frequency, motor.duty.min, motor.duty.max);
        }
        else if (motor.type == HardwareConfig::DriveMotor::ESC) {
            EasyKit::ServoConfig cfg;
            cfg.minUs = 1000;
            cfg.maxUs = 2000;
            cfg.centerUs = 1500;
            cfg.freq = (motor.frequency >= 40 && motor.frequency <= 900)
                           ? (uint16_t)motor.frequency : 50;
            if (esc->attach(motor.hardwareId, cfg) == EasyKit::Result::OK) {
                ch.attached = true;
                Serial.printf("[HardwareInit] Motor (servo/ESC): Pin=%d Freq=%dHz Duty=%d..%d%%\n",
                              motor.hardwareId, cfg.freq, motor.duty.min, motor.duty.max);
            } else {
                ch.attached = false;
                Serial.printf("[HardwareInit] Motor ESC attach FAILED on Pin=%d\n",
                              motor.hardwareId);
            }
        }
    }

    static void initSteeringServos(const HardwareConfig& hw) {
        s_steeringServoCount = 0;
        s_lastServoPos = -999;
        uint8_t effSteeringCount = hw.steeringServoCount;
        if (effSteeringCount == 0 && hw.steeringServos[0].configured) {
            effSteeringCount = 1;
        }
        for (uint8_t i = 0; i < effSteeringCount && i < HardwareConfig::MAX_STEERING_SERVOS; i++) {
            const auto& servo = hw.steeringServos[i];
            if (!servo.configured) continue;
            s_steeringConfigs[i] = servo;

            EasyKit::ServoConfig cfg;
            cfg.minUs = 500;
            cfg.maxUs = 2500;
            cfg.centerUs = servo.endpoints.center;
            cfg.freq = (servo.frequency >= 40 && servo.frequency <= 400) ? servo.frequency : 50;

            if (steeringServos[i].attach(servo.hardwareId, cfg) == EasyKit::Result::OK) {
                s_steeringServoCount++;
                Serial.printf("[HardwareInit] Steering servo[%d]: Pin=%d Freq=%dHz Center=%dus (L=%d, R=%d)\n",
                              i, servo.hardwareId, cfg.freq, servo.endpoints.center, servo.endpoints.left, servo.endpoints.right);
                steeringServos[i].writeMicroseconds(servo.endpoints.center);
            } else {
                Serial.printf("[HardwareInit] Steering servo[%d] attach FAILED on Pin=%d\n", i, servo.hardwareId);
            }
        }
    }

    static void initLights(const HardwareConfig::Lights& lights) {
        headPinCount = 0; fullPinCount = 0; fogPinCount = 0;
        tailPinCount = 0; brakePinCount = 0; reversingPinCount = 0;
        beaconPinCount = 0; cabPinCount = 0; workPinCount = 0;
        stepPinCount = 0; auxPinCount = 0;
        turnLPin = 0xFF; turnRPin = 0xFF; ditchLPin = 0xFF; ditchRPin = 0xFF;

        const EasyKit::LEDConfig cfg = {5000, EasyKit::LEDCResolution::Bits10, -1, false};

        auto initGroup = [&](const HardwareConfig::Lights::Light& light, EasyLED* leds, uint8_t* pins, uint8_t& count, const char* name) {
            count = 0;
            if (!light.configured) return;
            uint8_t effectiveCount = light.pinCount;
            if (effectiveCount == 0 && light.pin != 0xFF) effectiveCount = 1;
            for (uint8_t p = 0; p < effectiveCount && p < HardwareConfig::MAX_PINS_PER_LIGHT; p++) {
                pins[p] = (p < light.pinCount) ? light.pins[p] : light.pin;
                leds[p].begin(pins[p], cfg);
                count++;
                Serial.printf("[HardwareInit] %s[%d]: Pin=%d Brightness=%d%%\n", name, p, pins[p], light.brightness);
            }
        };

        initGroup(lights.headLight, headLeds, headPins, headPinCount, "Headlight");
        initGroup(lights.fullBeam, fullLeds, fullPins, fullPinCount, "Full beam");
        initGroup(lights.fogLamp, fogLeds, fogPins, fogPinCount, "Fog lamp");
        initGroup(lights.tailLight, tailLeds, tailPins, tailPinCount, "Taillight");
        initGroup(lights.brakeLight, brakeLeds, brakePins, brakePinCount, "Brakelight");
        initGroup(lights.beacon, beaconLeds, beaconPins, beaconPinCount, "Beacon");
        initGroup(lights.cabLight, cabLeds, cabPins, cabPinCount, "Cab light");
        initGroup(lights.workLight, workLeds, workPins, workPinCount, "Work light");
        initGroup(lights.stepLight, stepLeds, stepPins, stepPinCount, "Step light");
        initGroup(lights.auxLight, auxLeds, auxPins, auxPinCount, "Aux light");

        if (lights.reversingLight.configured) {
            reversingPinCount = 0;
            uint8_t effectiveCount = lights.reversingLight.pinCount;
            if (effectiveCount == 0 && lights.reversingLight.pin != 0xFF) effectiveCount = 1;
            for (uint8_t p = 0; p < effectiveCount && p < HardwareConfig::MAX_PINS_PER_LIGHT; p++) {
                uint8_t pin = (p < lights.reversingLight.pinCount) ? lights.reversingLight.pins[p] : lights.reversingLight.pin;
                reversingPins[p] = pin;
                if (!findLight(pin)) {
                    reversingLeds[p].begin(pin, cfg);
                }
                reversingPinCount++;
                Serial.printf("[HardwareInit] Reversing light[%d]: Pin=%d\n", p, pin);
            }
        }

        if (lights.ditchLight.configured) {
            ditchLPin = lights.ditchLight.leftPin;
            ditchRPin = lights.ditchLight.rightPin;
            s_ditchBrightness = lights.ditchLight.brightness;
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

        if (lights.turnLight.configured) {
            turnLPin = lights.turnLight.leftPin;
            turnRPin = lights.turnLight.rightPin;
            if (turnLPin != 0xFF) turnLLed.begin(turnLPin, cfg);
            if (turnRPin != 0xFF) turnRLed.begin(turnRPin, cfg);
            Serial.printf("[HardwareInit] Turn signals: L=%d R=%d Interval=%d/%dms\n",
                          turnLPin, turnRPin,
                          lights.turnLight.intervalOn, lights.turnLight.intervalOff);
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

HardwareInit::MotorChannel HardwareInit::s_driveCh[HardwareConfig::MAX_DRIVE_MOTORS];
uint8_t HardwareInit::s_driveChCount = 0;
HardwareInit::MotorChannel HardwareInit::s_rightCh;
uint8_t HardwareInit::s_primaryBemfPin = 0xFF;

uint8_t HardwareInit::headPins[HardwareConfig::MAX_PINS_PER_LIGHT];
uint8_t HardwareInit::headPinCount = 0;
uint8_t HardwareInit::fullPins[HardwareConfig::MAX_PINS_PER_LIGHT];
uint8_t HardwareInit::fullPinCount = 0;
uint8_t HardwareInit::fogPins[HardwareConfig::MAX_PINS_PER_LIGHT];
uint8_t HardwareInit::fogPinCount = 0;
uint8_t HardwareInit::tailPins[HardwareConfig::MAX_PINS_PER_LIGHT];
uint8_t HardwareInit::tailPinCount = 0;
uint8_t HardwareInit::brakePins[HardwareConfig::MAX_PINS_PER_LIGHT];
uint8_t HardwareInit::brakePinCount = 0;
uint8_t HardwareInit::reversingPins[HardwareConfig::MAX_PINS_PER_LIGHT];
uint8_t HardwareInit::reversingPinCount = 0;
uint8_t HardwareInit::beaconPins[HardwareConfig::MAX_PINS_PER_LIGHT];
uint8_t HardwareInit::beaconPinCount = 0;
uint8_t HardwareInit::cabPins[HardwareConfig::MAX_PINS_PER_LIGHT];
uint8_t HardwareInit::cabPinCount = 0;
uint8_t HardwareInit::workPins[HardwareConfig::MAX_PINS_PER_LIGHT];
uint8_t HardwareInit::workPinCount = 0;
uint8_t HardwareInit::stepPins[HardwareConfig::MAX_PINS_PER_LIGHT];
uint8_t HardwareInit::stepPinCount = 0;
uint8_t HardwareInit::auxPins[HardwareConfig::MAX_PINS_PER_LIGHT];
uint8_t HardwareInit::auxPinCount = 0;

uint8_t HardwareInit::turnLPin = 0xFF;
uint8_t HardwareInit::turnRPin = 0xFF;
uint8_t HardwareInit::ditchLPin = 0xFF;
uint8_t HardwareInit::ditchRPin = 0xFF;

HardwareConfig::AuxMotor HardwareInit::s_auxConfigs[HardwareConfig::MAX_AUX_MOTORS];
uint8_t                  HardwareInit::s_auxMotorCount = 0;

HardwareConfig::SteeringServo HardwareInit::s_steeringConfigs[HardwareConfig::MAX_STEERING_SERVOS];
uint8_t                       HardwareInit::s_steeringServoCount = 0;

float    HardwareInit::s_easingSpeedDegS = 180.0f;
float    HardwareInit::s_easingKIn = 0.2f;
float    HardwareInit::s_easingKOut = 0.8f;
uint16_t HardwareInit::s_fadeDurationMs = 250;
uint8_t  HardwareInit::s_ditchBrightness = 100;

uint8_t  HardwareInit::s_blinkPin[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
bool     HardwareInit::s_blinkActive[4] = { false, false, false, false };

EasyMotor HardwareInit::driveMotor;
EasyServo HardwareInit::steeringServos[HardwareConfig::MAX_STEERING_SERVOS];
EasyServo HardwareInit::escServo;
EasyServo HardwareInit::auxServo;
EasyMotor HardwareInit::auxMotor;

EasyLED   HardwareInit::headLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
EasyLED   HardwareInit::fullLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
EasyLED   HardwareInit::fogLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
EasyLED   HardwareInit::tailLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
EasyLED   HardwareInit::brakeLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
EasyLED   HardwareInit::reversingLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
EasyLED   HardwareInit::beaconLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
EasyLED   HardwareInit::cabLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
EasyLED   HardwareInit::workLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
EasyLED   HardwareInit::stepLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
EasyLED   HardwareInit::auxLeds[HardwareConfig::MAX_PINS_PER_LIGHT];

EasyLED   HardwareInit::turnLLed;
EasyLED   HardwareInit::turnRLed;
EasyLED   HardwareInit::ditchLLed;
EasyLED   HardwareInit::ditchRLed;
EasyLEDGroup HardwareInit::s_ditchGroup;
bool      HardwareInit::s_ditchActive = false;
uint8_t   HardwareInit::s_drivetrainType = HardwareConfig::ACKERMANN;
int16_t   HardwareInit::s_lastServoPos = -999;
