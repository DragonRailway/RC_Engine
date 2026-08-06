#pragma once

#include <Arduino.h>
#include "Config.h"
#include "PinMapper.h"
#include <EasyMotor.h>
#include <EasyServo.h>
#include <EasyLED.h>

class HardwareInit {
public:
    static void init(const HardwareConfig& hw) {
        Serial.println("[HardwareInit] Initializing peripherals...");

        s_drivetrainType = hw.drivetrainType;
        if (hw.drivetrainType == HardwareConfig::SKID_STEER) {
            initDriveMotor(hw.leftMotor);
            initDriveMotor(hw.rightMotor);
        } else {
            initDriveMotor(hw.driveMotor);
            initSteeringServo(hw.steeringServo);
        }
        initLights(hw.lights);

        Serial.println("[HardwareInit] Done");
    }

    static void hotReload(const HardwareConfig& hw) {
        Serial.println("[HardwareInit] Hot-reloading...");
        stopAll();
        init(hw);
    }

    static void stopAll() {
        // Release all EasyKit hardware so hotReload can re-attach cleanly.
        driveMotor.end();
        steeringServo.detach();
        escServo.detach();
        headLed.end();
        tailLed.end();
        brakeLed.end();
        turnLLed.end();
        turnRLed.end();
        reversingLed.end();
        motorAttached = false;
        Serial.println("[HardwareInit] Stopped all outputs");
    }

    static void setSkidMotors(int16_t leftSpeed, int16_t rightSpeed) {
        setMotor(leftSpeed);
    }

    // ────────────────────────────────────────────────────────────────
    // Runtime control API (used by VehicleController)
    // ────────────────────────────────────────────────────────────────

    // speed: -100 (full reverse) .. +100 (full forward)
    static void setMotor(int16_t speed) {
        if (motorType == HardwareConfig::DriveMotor::NONE || !motorAttached) return;

        int16_t eff = speed;
        switch (motorDirection) {
            case HardwareConfig::DriveMotor::REVERSE:    eff = -eff;     break;
            case HardwareConfig::DriveMotor::UNI_FORWARD: eff = abs(eff); break;
            case HardwareConfig::DriveMotor::UNI_REVERSE: eff = -abs(eff); break;
            default: break;   // FORWARD
        }

        // Duty percent within configured min..max window; 0 when neutral
        uint8_t pct = 0;
        if (eff > 0) {
            pct = motorDutyMin + (uint8_t)((uint32_t)eff * (motorDutyMax - motorDutyMin) / 100);
        } else if (eff < 0) {
            pct = motorDutyMin + (uint8_t)((uint32_t)(-eff) * (motorDutyMax - motorDutyMin) / 100);
        }
        if (pct > motorDutyMax) pct = motorDutyMax;

        if (motorType == HardwareConfig::DriveMotor::ESC) {
            // PPM pulse: 1000..2000us, neutral 1500us, deadband around center
            uint16_t us = 1500;
            if (abs(eff) >= 5) us = (uint16_t)(1500 + (int32_t)eff * 500 / 100);
            if (us < 1000) us = 1000;
            if (us > 2000) us = 2000;
            escServo.writeMicroseconds(us);
            return;
        }

        // H-Bridge via EasyMotor: signed percent, direction handled by the driver
        driveMotor.write(eff >= 0 ? (float)pct : -(float)pct);
    }

    // position: -100 (left) .. +100 (right), 0 = center
    static void setServo(int16_t position) {
        if (servoPin == 0xFF || !steeringServo.attached()) return;

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

private:
    static uint8_t s_drivetrainType;
    static uint8_t motorPwm1Pin;
    static uint8_t motorPwm2Pin;
    static uint8_t motorEnablePin;
    static uint8_t servoPin;
    static uint8_t escPin;

    // Light pins (tracked for setLight routing + hotReload teardown)
    static uint8_t headPin;
    static uint8_t tailPin;
    static uint8_t brakePin;
    static uint8_t turnLPin;
    static uint8_t turnRPin;
    static uint8_t reversingPin;

    // Runtime control state captured from config at init
    static uint8_t  motorType;        // HardwareConfig::DriveMotor::Type
    static uint8_t  motorDirection;   // HardwareConfig::DriveMotor::Direction
    static uint8_t  motorDutyMin;
    static uint8_t  motorDutyMax;
    static bool     motorAttached;
    static uint32_t motorFrequency;

    static uint32_t servoFrequency;
    static uint16_t servoLeft;
    static uint16_t servoRight;
    static uint16_t servoCenter;

    // EasyKit hardware objects
    static EasyMotor driveMotor;
    static EasyServo steeringServo;
    static EasyServo escServo;      // used when drive motor type == ESC (PPM output)
    static EasyLED   headLed;
    static EasyLED   tailLed;
    static EasyLED   brakeLed;
    static EasyLED   turnLLed;
    static EasyLED   turnRLed;
    static EasyLED   reversingLed;

    static EasyLED* findLight(uint8_t pin) {
        if (pin == headPin     && headLed.isAttached())     return &headLed;
        if (pin == tailPin     && tailLed.isAttached())     return &tailLed;
        if (pin == brakePin    && brakeLed.isAttached())    return &brakeLed;
        if (pin == turnLPin    && turnLLed.isAttached())    return &turnLLed;
        if (pin == turnRPin    && turnRLed.isAttached())    return &turnRLed;
        if (pin == reversingPin && reversingLed.isAttached()) return &reversingLed;
        return nullptr;
    }

    static void initDriveMotor(const HardwareConfig::DriveMotor& motor) {
        if (motor.type == HardwareConfig::DriveMotor::NONE) {
            motorAttached = false;
            Serial.println("[HardwareInit] No drive motor configured");
            return;
        }

        motorType = motor.type;
        motorDirection = motor.direction;
        motorDutyMin = motor.duty.min;
        motorDutyMax = motor.duty.max;
        motorFrequency = motor.frequency;

        if (motor.type == HardwareConfig::DriveMotor::HBRIDGE) {
            const char* name = (motor.hardwareId == PinMapper::BRIDGE_A) ? "HBRIDGE_A" : "HBRIDGE_B";
            HBridgePins pins = PinMapper::getHBridge(name);

            motorPwm1Pin = pins.pwm1;
            motorPwm2Pin = pins.pwm2;
            motorEnablePin = pins.enable;

            if (pins.dualPwm) {
                // Dual-PWM bridge (HBRIDGE_A): both pins are PWM
                driveMotor.begin(EasyMotor::DriverType::DRIVER_2PWM,
                                 pins.pwm1, pins.pwm2, pins.enable, false);
            } else {
                // DIR + PWM bridge (HBRIDGE_B): pin1 = speed PWM, pin2 = direction.
                // EasyKit drives DIR HIGH for forward; this bridge drives DIR LOW for
                // forward, so invert to preserve the previous polarity.
                driveMotor.begin(EasyMotor::DriverType::DRIVER_1PWM_1DIR,
                                 pins.pwm1, pins.pwm2, pins.enable, true);
            }
            driveMotor.setFrequency(motorFrequency);
            motorAttached = true;

            Serial.printf("[HardwareInit] H-Bridge: PWM1=%d PWM2=%d EN=%d Freq=%dHz\n",
                          motorPwm1Pin, motorPwm2Pin, motorEnablePin, motor.frequency);
        }
        else if (motor.type == HardwareConfig::DriveMotor::ESC) {
            escPin = motor.hardwareId;

            EasyKit::ServoConfig cfg;
            cfg.minUs = 1000;
            cfg.maxUs = 2000;
            cfg.centerUs = 1500;
            // ESC runs on a servo-style PPM pulse; fall back to 50 Hz if the
            // config frequency is not a sane servo refresh rate.
            cfg.freq = (motorFrequency >= 40 && motorFrequency <= 900)
                           ? (uint16_t)motorFrequency : 50;

            if (escServo.attach(escPin, cfg) == EasyKit::Result::OK) {
                motorAttached = true;
                Serial.printf("[HardwareInit] ESC: Pin=%d Freq=%dHz\n", escPin, cfg.freq);
            } else {
                Serial.printf("[HardwareInit] ESC attach FAILED on Pin=%d\n", escPin);
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

        EasyKit::ServoConfig cfg;
        cfg.minUs = servoLeft;
        cfg.maxUs = servoRight;
        cfg.centerUs = servoCenter;
        cfg.freq = servoFrequency;

        if (steeringServo.attach(servoPin, cfg) == EasyKit::Result::OK) {
            Serial.printf("[HardwareInit] Servo: Pin=%d Freq=%dHz Center=%dus\n",
                          servoPin, servo.frequency, servo.endpoints.center);
        } else {
            Serial.printf("[HardwareInit] Servo attach FAILED on Pin=%d\n", servoPin);
        }
    }

    static void initLights(const HardwareConfig::Lights& lights) {
        headPin = 0xFF; tailPin = 0xFF; brakePin = 0xFF;
        turnLPin = 0xFF; turnRPin = 0xFF; reversingPin = 0xFF;

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
};

uint8_t HardwareInit::motorPwm1Pin = 0xFF;
uint8_t HardwareInit::motorPwm2Pin = 0xFF;
uint8_t HardwareInit::motorEnablePin = 0xFF;
uint8_t HardwareInit::servoPin = 0xFF;
uint8_t HardwareInit::escPin = 0xFF;

uint8_t HardwareInit::headPin = 0xFF;
uint8_t HardwareInit::tailPin = 0xFF;
uint8_t HardwareInit::brakePin = 0xFF;
uint8_t HardwareInit::turnLPin = 0xFF;
uint8_t HardwareInit::turnRPin = 0xFF;
uint8_t HardwareInit::reversingPin = 0xFF;

uint8_t  HardwareInit::motorType = HardwareConfig::DriveMotor::NONE;
uint8_t  HardwareInit::motorDirection = HardwareConfig::DriveMotor::FORWARD;
uint8_t  HardwareInit::motorDutyMin = 20;
uint8_t  HardwareInit::motorDutyMax = 90;
bool     HardwareInit::motorAttached = false;
uint32_t HardwareInit::motorFrequency = 20000;

uint32_t HardwareInit::servoFrequency = 50;
uint16_t HardwareInit::servoLeft = 1350;
uint16_t HardwareInit::servoRight = 1650;
uint16_t HardwareInit::servoCenter = 1500;

EasyMotor HardwareInit::driveMotor;
EasyServo HardwareInit::steeringServo;
EasyServo HardwareInit::escServo;
EasyLED   HardwareInit::headLed;
EasyLED   HardwareInit::tailLed;
EasyLED   HardwareInit::brakeLed;
EasyLED   HardwareInit::turnLLed;
EasyLED   HardwareInit::turnRLed;
EasyLED   HardwareInit::reversingLed;
uint8_t   HardwareInit::s_drivetrainType = HardwareConfig::ACKERMANN;
