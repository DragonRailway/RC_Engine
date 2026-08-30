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
    static void init(const HardwareConfig& hw);
    static void hotReload(const HardwareConfig& hw);
    static void stopAll();
    static void update(uint16_t buttonHoldS = 4, uint8_t indicatorPin = 0xFF);
    static void latchPower(uint16_t bootLatchS = 1);
    static void updatePowerButton(uint16_t buttonHoldS = 4, uint8_t indicatorPin = 0xFF);

    static bool consumeButtonClicked() {
        bool clicked = s_buttonClicked;
        s_buttonClicked = false;
        return clicked;
    }

    static bool isCharging() {
        if (POWER::CHARGE_SENS == 0xFF) return false;
        return digitalRead(POWER::CHARGE_SENS) == LOW;
    }

    static void powerOff();

    static bool isPowerLatched() { return s_powerLatched; }

    static void setSkidMotors(int16_t leftSpeed, int16_t rightSpeed);
    static void setAllMotors(int16_t speed);
    static void setMotor(int16_t speed);

    static uint8_t getPrimaryBemfPin() {
        return s_primaryBemfPin;
    }

    static void setChannel(MotorChannel& ch, int16_t speed);
    static void setServo(int16_t position);
    static void setLight(uint8_t pin, uint8_t brightnessPct);
    static void setLight(const HardwareConfig::Lights::Light& light, uint8_t brightnessPct);
    static void setLightBlink(uint8_t pin, bool active, uint16_t onMs, uint16_t offMs, uint8_t dutyPct);
    static void setLightFade(uint8_t pin, uint8_t targetPct, uint16_t durationMs);
    static void setLightFade(const HardwareConfig::Lights::Light& light, uint8_t targetPct, uint16_t durationMs);

    static uint8_t getLightDutyPercent(uint8_t pin) {
        if (pin == 0xFF || pin == 0) return 0;
        EasyLED* led = findLight(pin);
        if (!led) return 0;
        return (uint8_t)led->getDutyPercent();
    }

    static void stopLightAnimations();
    static void setDitchLights(bool active, uint16_t intervalMs);
    static void setAuxMotor(int16_t speed);
    static void setAuxLight(uint8_t brightnessPct);
    static void setBeacon(bool active, uint16_t intervalMs = 120);
    static uint8_t getConfiguredLightMask(const HardwareConfig::Lights& L, bool isLoco);

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
    static constexpr uint8_t MAX_PHYSICAL_DRIVERS = 4;

    static EasyMotor s_motorDrivers[MAX_PHYSICAL_DRIVERS];
    static EasyServo steeringServos[HardwareConfig::MAX_STEERING_SERVOS];
    static EasyServo escServo;
    static EasyServo auxServos[HardwareConfig::MAX_AUX_MOTORS];

    static EasyMotor* getDriverForId(uint8_t hardwareId);
    static const char* getDriverName(uint8_t hardwareId);

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

    static void initAuxOutputs(const HardwareConfig& hw);
    static EasyLED* findLight(uint8_t pin);
    static void resetBlinkTracking();
    static int8_t blinkSlot(uint8_t pin);
    static void initChannel(MotorChannel& ch, const HardwareConfig::DriveMotor& motor,
                            EasyMotor* driver, EasyServo* esc);
    static void initSteeringServos(const HardwareConfig& hw);
    static void initLights(const HardwareConfig::Lights& lights);

private:
    static bool     s_powerLatched;
    static bool     s_powerButtonHolding;
    static uint32_t s_powerButtonHoldStart;
    static bool     s_buttonClicked;
    static int16_t  s_lastServoPos;
};
