#pragma once

#include <Arduino.h>

struct HardwareConfig {
    static constexpr uint8_t MAX_DRIVE_MOTORS    = 2;
    static constexpr uint8_t MAX_STEERING_SERVOS = 2;
    static constexpr uint8_t MAX_AUX_MOTORS       = 2;
    static constexpr uint8_t MAX_PINS_PER_LIGHT  = 4;

    char name[32] = "";
    char description[128] = "";

    enum DrivetrainType { ACKERMANN, SKID_STEER } drivetrainType = ACKERMANN;

    struct Sound {
        uint8_t volume = 80;
        bool    configured = false;
    } sound;

    struct DriveMotor {
        enum Type { NONE, DRIVER, ESC } type = NONE;
        uint8_t hardwareId = 0;
        uint16_t frequency = 20000;
        enum Direction { FORWARD, REVERSE, UNI_FORWARD, UNI_REVERSE } direction = FORWARD;
        struct Duty {
            uint8_t min = 10;
            uint8_t max = 100;
        } duty;
        bool    configured = false;
    };

    DriveMotor driveMotors[MAX_DRIVE_MOTORS];
    uint8_t    driveMotorCount = 0;
    DriveMotor leftMotor, rightMotor; // used when drivetrainType == SKID_STEER

    uint8_t steeringSensitivity = 80;

    struct SteeringServo {
        uint8_t hardwareId = 0;
        uint16_t frequency = 50;
        struct Endpoints {
            uint16_t left = 1400;
            uint16_t right = 1600;
            uint16_t center = 1500;
        } endpoints;
        bool    configured = false;
    };

    SteeringServo steeringServos[MAX_STEERING_SERVOS];
    uint8_t       steeringServoCount = 0;

    // Animation tunables for the EasyKit easing/fade/blink engines. Global
    // defaults; absent from a hardware config, these values apply as-is.
    struct Animation {
        float    easingSpeedDegS = 180.0f; // aux-servo move speed (0 = instant)
        float    easingKIn       = 0.2f;   // easing strength at move start
        float    easingKOut      = 0.8f;   // easing strength at move end
        uint16_t fadeDurationMs  = 0;      // headlight fade transition time (0 = instant)
        bool     configured = false;
    } animation;

    struct Lights {
        struct Light {
            uint8_t pins[MAX_PINS_PER_LIGHT] = {0};
            uint8_t pinCount = 0;
            uint8_t brightness = 60;
            bool configured = false;
            uint8_t pin = 0xFF; // primary pin (pins[0])
        };

        Light headLight;
        Light fullBeam;
        Light fogLamp;
        Light tailLight;
        Light brakeLight;
        Light reversingLight;
        Light beacon;
        Light cabLight;
        Light workLight;
        Light stepLight;
        Light auxLight;

        // Auxiliary locomotive lights — app-toggleable (loco light selector
        // items F/G/H, bits 5/6/7).
        struct DitchLight {
            uint8_t leftPin = 0;
            uint8_t rightPin = 0;
            uint8_t brightness = 100;
            uint16_t intervalMs = 8;  // alternation half-period: each side on for this long
            bool configured = false;
        } ditchLight;  // two outputs flashing alternately (left/right)

        struct TurnLight {
            uint8_t leftPin = 0;
            uint8_t rightPin = 0;
            uint8_t brightness = 60;
            uint16_t intervalOn = 300;
            uint16_t intervalOff = 300;
            bool configured = false;
        } turnLight;

    } lights;

    // Auxiliary outputs (work machines): aux_motors (mixer/tipper/trailer_dcc)
    // and aux_light. Both are optional. The hardware token decides the electrical kind
    // (DRIVER_* → H-bridge, S* → servo/ESC PPM, L* → LED), exactly like
    // drive_motors; `type` is the aux *purpose* (drive behavior), not the wiring.
    struct AuxMotor {
        enum Purpose { NONE, MIXER, TIPPER, TRAILER_DCC } purpose = NONE;
        DriveMotor motor;   // electrical shape: hardware/frequency/direction/duty
        bool    configured = false;
    };

    AuxMotor auxMotors[MAX_AUX_MOTORS];
    uint8_t  auxMotorCount = 0;

    struct AuxLight {
        uint8_t pins[MAX_PINS_PER_LIGHT] = {0};
        uint8_t pinCount = 0;
        uint8_t brightness = 60;
        bool configured = false;
        uint8_t pin = 0xFF; // primary pin (pins[0])
    } auxLight;

    // Battery / power configuration. cellCount is the single source of truth for
    // the connected LiPo pack (1S, 2S, 3S, 4S). When cellCount is 0 the firmware
    // falls back to voltage-based auto-detection at boot (legacy behavior).
    // vScale/vOffset calibrate the voltage sense ADC: when present in the config
    // they override the compile-time VSCALE/VOFFSET macros (platformio.ini).
    struct Battery {
        uint8_t cellCount = 0;        // 0 = auto-detect, otherwise fixed (1..4)
        float   warningVoltage = 3.5f;// low-voltage warning threshold per cell (V)
        float   cutoffVoltage = 3.3f; // low-voltage cutoff per cell (V)
        float   fullVoltage = 4.2f;   // fully-charged voltage per cell (V)
        float   vScale = 1.0f;        // voltage sense scale (defaults to VSCALE)
        float   vOffset = 0.0f;       // voltage sense offset (defaults to VOFFSET)
        bool    configured = false;
    } battery;

    struct Power {
        uint8_t  indicatorPin = 0xFF;      // optional power indicator pin / light alias
        uint16_t bootLatchS = 1;          // boot button hold to latch ON (seconds)
        uint16_t buttonHoldS = 4;         // runtime long-press hold to power OFF (seconds)
        uint16_t disconnectTimeoutS = 60; // idle/disconnect auto-off timeout (seconds, 0 = disabled)
        uint16_t warningWindowS = 10;     // warning phase before disconnect auto-off (seconds)
        uint16_t cutoffDelayS = 1;        // delay under low-voltage cutoff before power OFF (seconds)
        bool     configured = false;
    } power;

    struct Charging {
        uint8_t pin = 0xFF;               // pin token or aliased light pin
        uint8_t mode = 0;                 // 0=solid, 1=blink, 2=pulse
        bool    configured = false;
    } charging;

};
