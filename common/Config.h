#pragma once

#include <Arduino.h>

struct HardwareConfig {
    enum DrivetrainType { ACKERMANN, SKID_STEER } drivetrainType = ACKERMANN;

    struct Sound {
        uint8_t volume = 80;
    } sound;

    struct DriveMotor {
        enum Type { NONE, HBRIDGE, ESC } type = NONE;
        uint8_t hardwareId = 0;
        uint16_t frequency = 20000;
        enum Direction { FORWARD, REVERSE, UNI_FORWARD, UNI_REVERSE } direction = FORWARD;
        struct Duty {
            uint8_t min = 20;
            uint8_t max = 90;
        } duty;
    } driveMotor, leftMotor, rightMotor;

    uint8_t steeringSensitivity = 80;

    struct SteeringServo {
        uint8_t hardwareId = 0;
        uint16_t frequency = 50;
        struct Endpoints {
            uint16_t left = 1350;
            uint16_t right = 1650;
            uint16_t center = 1500;
        } endpoints;
    } steeringServo;

    // Animation tunables for the EasyKit easing/fade/blink engines. Global
    // defaults; absent from a hardware config, these values apply as-is.
    struct Animation {
        float    easingSpeedDegS = 180.0f; // aux-servo move speed (0 = instant)
        float    easingKIn       = 0.2f;   // easing strength at move start
        float    easingKOut      = 0.8f;   // easing strength at move end
        uint16_t fadeDurationMs  = 250;    // headlight fade transition time
    } animation;

    struct Lights {
        struct Light {
            uint8_t pin = 0;
            uint8_t brightness = 60;
            bool configured = false;
        };

        Light headLight;
        Light tailLight;
        Light brakeLight;

        struct TurnLight {
            uint8_t leftPin = 0;
            uint8_t rightPin = 0;
            uint8_t brightness = 60;
            uint16_t intervalOn = 500;
            uint16_t intervalOff = 500;
            bool configured = false;
        } turnLight;

        Light reversingLight;
    } lights;

    struct Telemetry {
        float vScale = 1.0f;
        float vOffset = 0.0f;
    } telemetry;

    // Battery / power configuration. cellCount is the single source of truth for
    // the connected LiPo pack (1S, 2S, 3S, 4S). When cellCount is 0 the firmware
    // falls back to voltage-based auto-detection at boot (legacy behavior).
    struct Battery {
        uint8_t cellCount = 0;        // 0 = auto-detect, otherwise fixed (1..4)
        float   cutoffVoltage = 3.3f; // low-voltage cutoff per cell (V)
        float   fullVoltage = 4.2f;   // fully-charged voltage per cell (V)
    } battery;
};
