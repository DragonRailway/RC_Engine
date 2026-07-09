#pragma once

#include <Arduino.h>

struct HardwareConfig {
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
    } driveMotor;

    struct SteeringServo {
        uint8_t hardwareId = 0;
        uint16_t frequency = 50;
        struct Endpoints {
            uint16_t left = 1350;
            uint16_t right = 1650;
            uint16_t center = 1500;
        } endpoints;
    } steeringServo;

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
};

struct VehicleConfig {
    char name[32] = {};
    char type[16] = {};

    struct Engine {
        uint8_t acc = 2;
        uint8_t dec = 1;
        uint8_t idleRpm = 10;
        uint16_t clutchRpm = 100;
        uint16_t revSwitchPoint = 50;
        uint16_t idleEndPoint = 40;
        uint8_t knockInterval = 8;
        uint8_t knockStartPoint = 30;
        uint8_t jakeBrakeMinRpm = 60;
        uint8_t fanStartPoint = 0;
    } engine;

    struct Transmission {
        enum Type { NONE, AUTOMATIC, MANUAL } type = NONE;
        uint8_t numberOfGears = 3;
    } transmission;

    struct SoundVolume {
        uint8_t start = 100;
        uint8_t idle = 100;
        uint8_t engineIdle = 50;
        uint8_t fullThrottle = 150;
        uint8_t rev = 100;
        uint8_t engineRev = 50;
        uint8_t turbo = 0;
        uint8_t knock = 0;
        uint8_t wastegate = 0;
        uint8_t horn = 100;
        uint8_t siren = 0;
        uint8_t brake = 0;
        uint8_t parkingBrake = 0;
        uint8_t shifting = 0;
        uint8_t reversing = 0;
        uint8_t indicator = 100;
        uint8_t coupling = 100;
        uint8_t jakeBrake = 0;
        uint8_t fan = 0;
    } soundVolume;
};
