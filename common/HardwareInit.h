#pragma once

#include <Arduino.h>
#include "Config.h"
#include "PinMapper.h"

class HardwareInit {
public:
    static void init(const HardwareConfig& hw) {
        Serial.println("[HardwareInit] Initializing peripherals...");

        initDriveMotor(hw.driveMotor);
        initSteeringServo(hw.steeringServo);
        initLights(hw.lights);

        Serial.println("[HardwareInit] Done");
    }

    static void hotReload(const HardwareConfig& hw) {
        Serial.println("[HardwareInit] Hot-reloading...");
        stopAll();
        init(hw);
    }

    static void stopAll() {
        if (motorPwm1Pin != 0xFF) {
            ledcWrite(motorPwm1Pin, 0);
            if (motorPwm2Pin != 0xFF) ledcWrite(motorPwm2Pin, 0);
        }
        if (servoPin != 0xFF) {
            ledcWrite(servoPin, 0);
        }
        Serial.println("[HardwareInit] Stopped all outputs");
    }

private:
    static uint8_t motorPwm1Pin;
    static uint8_t motorPwm2Pin;
    static uint8_t motorEnablePin;
    static uint8_t servoPin;

    static void initDriveMotor(const HardwareConfig::DriveMotor& motor) {
        if (motor.type == HardwareConfig::DriveMotor::NONE) {
            Serial.println("[HardwareInit] No drive motor configured");
            return;
        }

        if (motor.type == HardwareConfig::DriveMotor::HBRIDGE) {
            HBridgePins pins = PinMapper::getHBridge(
                motor.hardwareId == 12 ? "HBRIDGE_A" : "HBRIDGE_B"
            );

            motorPwm1Pin = pins.pwm1;
            motorPwm2Pin = pins.pwm2;
            motorEnablePin = 12;

            pinMode(motorEnablePin, OUTPUT);
            digitalWrite(motorEnablePin, HIGH);

            ledcAttach(motorPwm1Pin, motor.frequency, 8);
            ledcAttach(motorPwm2Pin, motor.frequency, 8);

            Serial.printf("[HardwareInit] H-Bridge: PWM1=%d PWM2=%d EN=%d Freq=%dHz\n",
                          motorPwm1Pin, motorPwm2Pin, motorEnablePin, motor.frequency);
        }
        else if (motor.type == HardwareConfig::DriveMotor::ESC) {
            motorPwm1Pin = motor.hardwareId;
            motorPwm2Pin = 0xFF;

            ledcAttach(motorPwm1Pin, motor.frequency, 16);

            Serial.printf("[HardwareInit] ESC: Pin=%d Freq=%dHz\n",
                          motorPwm1Pin, motor.frequency);
        }
    }

    static void initSteeringServo(const HardwareConfig::SteeringServo& servo) {
        if (servo.hardwareId == 0) {
            Serial.println("[HardwareInit] No steering servo configured");
            return;
        }

        servoPin = servo.hardwareId;
        ledcAttach(servoPin, servo.frequency, 16);

        Serial.printf("[HardwareInit] Servo: Pin=%d Freq=%dHz Center=%dus\n",
                      servoPin, servo.frequency, servo.endpoints.center);
    }

    static void initLights(const HardwareConfig::Lights& lights) {
        if (lights.headLight.configured) {
            ledcAttach(lights.headLight.pin, 5000, 10);
            Serial.printf("[HardwareInit] Headlight: Pin=%d Brightness=%d%%\n",
                          lights.headLight.pin, lights.headLight.brightness);
        }

        if (lights.tailLight.configured) {
            ledcAttach(lights.tailLight.pin, 5000, 10);
            Serial.printf("[HardwareInit] Taillight: Pin=%d Brightness=%d%%\n",
                          lights.tailLight.pin, lights.tailLight.brightness);
        }

        if (lights.brakeLight.configured) {
            ledcAttach(lights.brakeLight.pin, 5000, 10);
            Serial.printf("[HardwareInit] Brakelight: Pin=%d\n", lights.brakeLight.pin);
        }

        if (lights.turnLight.configured) {
            if (lights.turnLight.leftPin != 0xFF) {
                ledcAttach(lights.turnLight.leftPin, 5000, 10);
            }
            if (lights.turnLight.rightPin != 0xFF) {
                ledcAttach(lights.turnLight.rightPin, 5000, 10);
            }
            Serial.printf("[HardwareInit] Turn signals: L=%d R=%d Interval=%d/%dms\n",
                          lights.turnLight.leftPin, lights.turnLight.rightPin,
                          lights.turnLight.intervalOn, lights.turnLight.intervalOff);
        }

        if (lights.reversingLight.configured) {
            ledcAttach(lights.reversingLight.pin, 5000, 10);
            Serial.printf("[HardwareInit] Reversing light: Pin=%d\n", lights.reversingLight.pin);
        }
    }
};

uint8_t HardwareInit::motorPwm1Pin = 0xFF;
uint8_t HardwareInit::motorPwm2Pin = 0xFF;
uint8_t HardwareInit::motorEnablePin = 0xFF;
uint8_t HardwareInit::servoPin = 0xFF;
