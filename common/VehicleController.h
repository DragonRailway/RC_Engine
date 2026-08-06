#pragma once

#include <Arduino.h>
#include "RADIOKIT.h"
#include "Config.h"
#include "HardwareInit.h"
#include "boards.h"  // board selected at build time via platformio.ini env define
#include <RcEngineSound.h>
#include <VehicleProfile.h>

/**
 * @brief Vehicle control loop: maps RadioKit widget inputs to motor, servo,
 *        lights and the sound engine, and publishes telemetry.
 *
 * Header-only class following the project's common/ convention.
 * Input source depends on the active page:
 *   - Page 0 "Truck": gas_pedal (throttle), brake_pedal, steering_wheel, led_select
 *   - Page 1 "Loco":  slider (throttle), dir_switch, lights_toggle, horn
 */
class VehicleController {
public:
    static void init(HardwareConfig* hw, RcEngineSound* engine, VehicleProfile* profile) {
        s_hw = hw;
        s_engine = engine;
        s_profile = profile;

        // Master volume comes from the hardware config
        if (s_hw && s_profile) {
            s_profile->config.sound.master = s_hw->sound.volume;
        }

        s_lastLightBits = 0xFF;
        s_brakePrev = false;
        s_hornPrev = false;
        s_reversePrev = false;
        s_lastTelemetry = 0;
        strcpy(s_battBuf, "--");
        strcpy(s_speedBuf, "--");

        // Safe startup: center steering, stop motor
        HardwareInit::setServo(0);
        HardwareInit::setMotor(0);
    }

    static void update() {
        if (!s_hw || !s_engine || !s_profile) return;

        const uint8_t page = RadioKit.getActivePage();

        // ── Throttle input ──
        // Pedals spring to center/min so released = 0 or -100; clamp negatives.
        // Loco slider is non-latching (-100..+100).
        int16_t throttleInput = (page == 1) ? slider.rk.value : gas_pedal.rk.value;
        int16_t throttlePct = throttleInput > 0 ? throttleInput : 0;   // 0..100

        // ── Direction (Loco page) ──
        bool reverse = dir_switch.rk.state;
        if (reverse != s_reversePrev) {
            s_engine->triggerReversing(reverse);
            s_reversePrev = reverse;
        }

        // ── Motor & Steering ──
        int16_t motorSpeed = reverse ? -throttlePct : throttlePct;
        if (s_hw->drivetrainType == HardwareConfig::SKID_STEER) {
            int16_t steerVal = steering_wheel.rk.value;
            int16_t sens = s_hw->steeringSensitivity;
            int16_t leftSpeed = throttlePct + (steerVal * sens / 100);
            int16_t rightSpeed = throttlePct - (steerVal * sens / 100);
            leftSpeed = constrain(leftSpeed, -100, 100);
            rightSpeed = constrain(rightSpeed, -100, 100);
            if (reverse) { leftSpeed = -leftSpeed; rightSpeed = -rightSpeed; }
            HardwareInit::setSkidMotors(leftSpeed, rightSpeed);
        } else {
            HardwareInit::setMotor(motorSpeed);
            HardwareInit::setServo(steering_wheel.rk.value);
        }

        // ── Sound engine: throttle is in RPM units, sign = direction ──
        int16_t rpm = (int16_t)((int32_t)throttlePct * s_profile->config.engine.maxRpm / 100);
        s_engine->update(reverse ? -rpm : rpm);

        // ── Brake (Truck page pedal) ──
        bool brake = brake_pedal.rk.value > 20;   // pressed past a small deadband
        if (brake != s_brakePrev) {
            s_engine->triggerBrake(brake);
            s_brakePrev = brake;
        }

        // ── Horn (Loco page) ──
        bool hornActive = horn.rk.state;
        if (hornActive != s_hornPrev) {
            s_engine->triggerHorn(hornActive);
            s_hornPrev = hornActive;
        }

        // ── Lights: multi-select bitmask (A=1, B=2, C=4) ──
        uint8_t bits = (page == 1) ? lights_toggle.rk.value : led_select.rk.value;
        if (bits != s_lastLightBits) {
            applyLights(bits);
            s_lastLightBits = bits;
        }

        // ── Telemetry (bounded rate) ──
        uint32_t now = millis();
        if (now - s_lastTelemetry >= 500) {
            s_lastTelemetry = now;
            updateTelemetry(motorSpeed);
        }
    }

private:
    static HardwareConfig* s_hw;
    static RcEngineSound*  s_engine;
    static VehicleProfile* s_profile;

    static uint8_t  s_lastLightBits;
    static bool     s_brakePrev;
    static bool     s_hornPrev;
    static bool     s_reversePrev;
    static uint32_t s_lastTelemetry;
    static char     s_battBuf[8];
    static char     s_speedBuf[8];

    static void applyLights(uint8_t bits) {
        if (!s_hw) return;
        const HardwareConfig::Lights& L = s_hw->lights;
        const bool a = bits & 0x01;   // A → head light
        const bool b = bits & 0x02;   // B → tail light
        const bool c = bits & 0x04;   // C → brake light
        const bool d = bits & 0x08;   // D → turn lights (left+right)
        const bool e = bits & 0x10;   // E → reversing light
        HardwareInit::setLight(L.headLight.pin,       a ? L.headLight.brightness : 0);
        HardwareInit::setLight(L.tailLight.pin,       b ? L.tailLight.brightness : 0);
        HardwareInit::setLight(L.brakeLight.pin,      c ? L.brakeLight.brightness : 0);
        HardwareInit::setLight(L.turnLight.leftPin,   d ? L.turnLight.brightness : 0);
        HardwareInit::setLight(L.turnLight.rightPin,  d ? L.turnLight.brightness : 0);
        HardwareInit::setLight(L.reversingLight.pin,  e ? L.reversingLight.brightness : 0);
    }

    static void updateTelemetry(int16_t motorSpeed) {
        // Battery: voltage sense pin with board calibration (VSCALE/VOFFSET)
        float pinV = analogReadMilliVolts(POWER::VOLTAGE) / 1000.0f;
        float batV = pinV * (float)VSCALE + (float)VOFFSET;
        int pct = (int)((batV - 3.3f) / (4.2f - 3.3f) * 100.0f);
        pct = constrain(pct, 0, 100);
        snprintf(s_battBuf, sizeof(s_battBuf), "%d", pct);
        telemetry_Battery.rk.content = s_battBuf;

        // Speed estimate: drive command magnitude 0..100
        snprintf(s_speedBuf, sizeof(s_speedBuf), "%d", abs(motorSpeed));
        telemetry_Speed.rk.content = s_speedBuf;
    }
};

HardwareConfig* VehicleController::s_hw = nullptr;
RcEngineSound*  VehicleController::s_engine = nullptr;
VehicleProfile* VehicleController::s_profile = nullptr;

uint8_t  VehicleController::s_lastLightBits = 0xFF;
bool     VehicleController::s_brakePrev = false;
bool     VehicleController::s_hornPrev = false;
bool     VehicleController::s_reversePrev = false;
uint32_t VehicleController::s_lastTelemetry = 0;
char     VehicleController::s_battBuf[8] = "--";
char     VehicleController::s_speedBuf[8] = "--";
