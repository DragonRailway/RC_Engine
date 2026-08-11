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
 * Includes:
 *   - Auto LiPo cell detection & low-voltage safety cutoff
 *   - Steering auto turn-signal cancel & dynamic decel brake lights
 *   - Headlight 3-state stepping (Off, Low Beam 40%, High Beam 100%)
 *   - Synchronized hazard light flashing
 *   - Engine Start/Stop power state & physics-based Jake Brake / Wastegate FX
 *   - Work Machine Hydraulics, Hydraulic Pump Load Governor (+20% RPM bump),
 *     speed-dependent Track Rattle, Bucket Rattle, Dump Bed, & Physical Aux Servos.
 */
class VehicleController {
public:
    // Modular control variables for Work Machine Hydraulics & Actuators
    static int16_t aux_hydraulic1;
    static int16_t aux_hydraulic2;
    static bool    bucket_rattle_trigger;
    static bool    dump_bed_toggle;

    static void init(HardwareConfig* hw, RcEngineSound* engine, VehicleProfile* profile) {
        s_hw = hw;
        s_engine = engine;
        s_profile = profile;

        // Master volume comes from the hardware config
        if (s_hw && s_profile) {
            s_profile->config.sound.master = s_hw->sound.volume;
        }

        s_brakePrev = false;
        s_hornPrev = false;
        s_bellPrev = false;
        s_reversePrev = false;
        s_lastTelemetry = 0;
        s_prevThrottlePct = 0;
        s_decelBrakeTime = 0;
        s_headlightMode = 0;
        s_lastBit0State = false;
        s_autoTurnLeft = false;
        s_autoTurnRight = false;
        s_jakeBrakePrev = false;
        s_indicatorPrev = false;
        s_gearPrev = 1;             // default Park until the radio reports a selection
        s_parkingBrakePrev = false;
        aux_hydraulic1 = 0;
        aux_hydraulic2 = 0;
        bucket_rattle_trigger = false;
        dump_bed_toggle = false;

        strcpy(s_battBuf, "--");
        strcpy(s_speedBuf, "--");

        // Safe startup: center steering, stop motor, center aux servos
        HardwareInit::setServo(0);
        HardwareInit::setMotor(0);
        HardwareInit::setAuxServo1(0);
        HardwareInit::setAuxServo2(0);

        // ── Battery Cell Count: config-driven, voltage auto-detect as fallback ──
        // Board hardware config is authoritative (cell_count: 1..4). Only when the
        // config omits it (cell_count: 0) do we fall back to legacy voltage-based
        // detection so the cutoff still engages for unknown packs.
        float sumV = 0;
        for (int i = 0; i < 10; ++i) {
            float pinV = analogReadMilliVolts(POWER::VOLTAGE) / 1000.0f;
            sumV += pinV * s_hw->telemetry.vScale + s_hw->telemetry.vOffset;
            delay(5);
        }
        float bootV = sumV / 10.0f;

        const uint8_t cfgCells = s_hw->battery.cellCount;
        if (cfgCells > 0) {
            s_cellCount = cfgCells;
            Serial.printf("[VehicleController] Battery cell count from config: %dS (Boot V: %.2fV)\n",
                          s_cellCount, bootV);
        } else {
            // Legacy auto-detect: a 1S pack measures well below the old 2S floor.
            if (bootV < 4.5f)       s_cellCount = 1;
            else if (bootV < 8.4f)  s_cellCount = 2;
            else if (bootV < 12.6f) s_cellCount = 3;
            else                    s_cellCount = 4;
            Serial.printf("[VehicleController] Auto-detected %dS LiPo (Boot V: %.2fV)\n",
                          s_cellCount, bootV);
        }

        s_cutoffVoltage = s_cellCount * s_hw->battery.cutoffVoltage;
        s_lowVoltageStart = 0;
        s_batteryCutoff = false;

        Serial.printf("[VehicleController] Battery cutoff: %.2fV (%.2fV/cell)\n",
                      s_cutoffVoltage, s_hw->battery.cutoffVoltage);

        // ── Vehicle type boot visibility ──
        const RcEngineSound::VehicleType t = s_profile->config.type;
        if (t == RcEngineSound::VEHICLE_EXCAVATOR) {
            Serial.println("[VehicleController] Vehicle type: EXCAVATOR (control surface deferred — using truck widget set)");
        } else if (t == RcEngineSound::VEHICLE_LOCOMOTIVE) {
            Serial.println("[VehicleController] Vehicle type: LOCOMOTIVE");
        } else if (t == RcEngineSound::VEHICLE_UNKNOWN) {
            Serial.println("[VehicleController] Vehicle type: UNKNOWN (defaulting to truck widget set)");
        } else {
            Serial.println("[VehicleController] Vehicle type: TRUCK");
        }
    }

    static void update() {
        if (!s_hw || !s_engine || !s_profile) return;

        // Vehicle type from vehicle-config.json is the single source of truth:
        // TRUCK and LOCOMOTIVE widget sets are mutually exclusive, never active together.
        // Dispatch on the canonical enum: LOCOMOTIVE -> loco widget set; TRUCK, EXCAVATOR
        // (recognized stub, see init) and UNKNOWN all use the truck widget set.
        const RcEngineSound::VehicleType vtype = s_profile->config.type;
        const bool isLoco = (vtype == RcEngineSound::VEHICLE_LOCOMOTIVE);

        // ── Battery Protection & Low Voltage Cutoff ──
        float pinV = analogReadMilliVolts(POWER::VOLTAGE) / 1000.0f;
        float batV = pinV * s_hw->telemetry.vScale + s_hw->telemetry.vOffset;

        if (batV < s_cutoffVoltage) {
            if (s_lowVoltageStart == 0) s_lowVoltageStart = millis();
            else if (millis() - s_lowVoltageStart >= 1500) {
                s_batteryCutoff = true;
            }
        } else if (batV > (s_cutoffVoltage + 0.2f * s_cellCount)) {
            s_lowVoltageStart = 0;
            s_batteryCutoff = false;
        }

        if (s_batteryCutoff) {
            HardwareInit::setMotor(0);
            s_engine->triggerOutOfFuel(true);
            bool hazardFlash = (millis() / 333) % 2 == 0;
            // Minimal alarm state: zeroed bits so only the hazard flashes (and out-of-fuel sound) show.
            applyLightsWithAutomation(0, hazardFlash, hazardFlash, false, false, 0, false);
            updateTelemetry(0, batV);
            return;
        } else {
            s_engine->triggerOutOfFuel(false);
        }

        // ── Throttle input ──
        int16_t throttleInput = isLoco ? throttle_slider.rk.value : gas_pedal.rk.value;
        int16_t throttlePct = throttleInput > 0 ? throttleInput : 0;   // 0..100

        // ── Engine Start / Power State Machine ──
        // Engine power toggles are type-driven and exclusive: start_button (TRUCK)
        // or engine_button (LOCOMOTIVE). Latched toggles: ON = run, OFF = stop.
        RcEngineSound::EngineState eState = s_engine->getState();
        uint8_t bits = isLoco ? loco_light.rk.value : truck_light.rk.value;
        bool engineStartToggle = isLoco ? engine_button.rk.state : start_button.rk.state;

        if (eState == RcEngineSound::OFF) {
            if (engineStartToggle) {  // Strict per spec: the Engine Power toggle is the sole start trigger
                s_engine->startEngine();
            }
            HardwareInit::setMotor(0);
            throttlePct = 0;
        } else if (eState == RcEngineSound::STARTING) {
            if (!engineStartToggle) s_engine->stopEngine();  // toggled OFF mid-crank cancels the start
            HardwareInit::setMotor(0);
            throttlePct = 0;
        } else if (eState == RcEngineSound::RUNNING && !engineStartToggle) {
            s_engine->stopEngine();
        }

        // ── Direction / Gear (type-driven) ──
        // LOCOMOTIVE: dir_switch is the sole direction authority.
        // TRUCK: gear_switch radio group D=0 (Drive), P=1 (Park), R=2 (Reverse).
        bool reverse;
        bool parkingBrake = false;
        uint8_t gear = 1;   // default Park (safe) until the radio reports a selection
        if (isLoco) {
            reverse = dir_switch.rk.state;
        } else {
            uint8_t g = gear_switch.rk.value;
            if (g <= 2) gear = g;
            reverse = (gear == 2);      // R
            parkingBrake = (gear == 1); // P
            if (gear != s_gearPrev) {
                // Shifting sound on gear change, only while the engine is RUNNING
                // (SHIFTING is a one-shot voice, so one trigger plays the sample once).
                if (eState == RcEngineSound::RUNNING) s_engine->triggerShifting(true);
                s_gearPrev = gear;
            }
        }
        if (reverse != s_reversePrev) {
            s_engine->triggerReversing(reverse);
            s_reversePrev = reverse;
        }
        if (parkingBrake != s_parkingBrakePrev) {
            s_engine->triggerParkingBrake(parkingBrake);
            s_parkingBrakePrev = parkingBrake;
        }

        // ── Motor & Steering ──
        // Truck widgets are read only on truck configs: on a locomotive the steering
        // wheel / brake pedal don't exist as inputs, so pin them to neutral instead of
        // letting stale RadioKit values bleed into the loco drive path.
        int16_t steerVal = isLoco ? 0 : steering_wheel.rk.value;

        // Proportional brake blend: brake_pedal above a 20% deadband scales motor
        // output linearly to zero at full brake (Ackermann + skid-steer). The sound
        // engine RPM and light automation keep using the raw throttlePct.
        int16_t brakePct = isLoco ? 0 : brake_pedal.rk.value;
        int16_t motorThrottle = throttlePct;
        if (parkingBrake) {
            motorThrottle = 0;   // Park locks the motor regardless of throttle or brake
        } else if (brakePct > 20) {
            // Renormalize over the active range so the ramp is continuous at the deadband
            // edge (scale 1.0 at brakePct=20, 0.0 at full brake 100).
            motorThrottle = (int16_t)((int32_t)throttlePct * (100 - brakePct) / 80);
        }
        int16_t motorSpeed = reverse ? -motorThrottle : motorThrottle;

        if (eState == RcEngineSound::RUNNING) {
            if (s_hw->drivetrainType == HardwareConfig::SKID_STEER) {
                int16_t sens = s_hw->steeringSensitivity;
                int16_t leftSpeed = motorThrottle + (steerVal * sens / 100);
                int16_t rightSpeed = motorThrottle - (steerVal * sens / 100);
                leftSpeed = constrain(leftSpeed, -100, 100);
                rightSpeed = constrain(rightSpeed, -100, 100);
                if (reverse) { leftSpeed = -leftSpeed; rightSpeed = -rightSpeed; }
                // Park must lock the tracks: the differential mix above would otherwise
                // creep one track with steering applied, so zero both sides explicitly.
                if (parkingBrake) { leftSpeed = 0; rightSpeed = 0; }
                HardwareInit::setSkidMotors(leftSpeed, rightSpeed);
            } else {
                HardwareInit::setMotor(motorSpeed);
                HardwareInit::setServo(steerVal);
            }
        }

        // ── Work Machine Physical Servos ──
        // Truck page aux_slider drives the tipper/cement-mixer channel (Aux Servo 1 +
        // hydraulic flow sound + load governor); Loco page has no aux control.
        aux_hydraulic1 = isLoco ? 0 : aux_slider.rk.value;
        HardwareInit::setAuxServo1(aux_hydraulic1);
        HardwareInit::setAuxServo2(aux_hydraulic2);

        // ── Work Machine Sound FX & Engine Pump Load Governor ──
        bool hydraulicFlowActive = (abs(aux_hydraulic1) > 10 || abs(aux_hydraulic2) > 10 || dump_bed_toggle);
        s_engine->triggerHydraulicFlow(hydraulicFlowActive);

        if (dump_bed_toggle) {
            s_engine->triggerDumpBed(true);
        } else {
            s_engine->triggerDumpBed(false);
        }

        if (bucket_rattle_trigger) {
            s_engine->triggerBucketRattle(true);
            bucket_rattle_trigger = false; // single-shot trigger
        }

        // Hydraulic Pump continuous sound
        if (s_profile->config.features.hydraulicEnabled) {
            s_engine->triggerHydraulicPump(eState == RcEngineSound::RUNNING);
        }

        // Track Pin Rattle evaluation
        if (s_profile->config.features.trackRattleEnabled) {
            bool vehicleMoving = (throttlePct > 0 && eState == RcEngineSound::RUNNING);
            s_engine->triggerTrackRattle(vehicleMoving);
        }

        // Engine sound RPM calculation with +20% idle bump during hydraulic flow
        int16_t targetPct = throttlePct;
        if (hydraulicFlowActive && eState == RcEngineSound::RUNNING) {
            targetPct = max(targetPct, (int16_t)20);
        }
        int16_t rpm = (int16_t)((int32_t)targetPct * s_profile->config.engine.maxRpm / 100);
        s_engine->update(reverse ? -rpm : rpm);

        // ── Physics-Based Sound FX: Jake Brake ──
        uint16_t curRpm = s_engine->getRpm();
        bool jakeCondition = (throttlePct == 0 && curRpm > (s_profile->config.engine.maxRpm * 60 / 100));
        if (jakeCondition != s_jakeBrakePrev) {
            s_engine->triggerJakeBrake(jakeCondition);
            s_jakeBrakePrev = jakeCondition;
        }

        // ── Brake pedal ──
        bool brakePressed = brake_pedal.rk.value > 20;
        if (brakePressed != s_brakePrev) {
            s_engine->triggerBrake(brakePressed);
            s_brakePrev = brakePressed;
        }

        // ── Horn (TRUCK) / Bell (LOCOMOTIVE) ──
        // Type-driven: horn_button lives on the truck page, bell_button on the loco page.
        bool hornActive = !isLoco && horn_button.rk.state;
        if (hornActive != s_hornPrev) {
            s_engine->triggerHorn(hornActive);
            s_hornPrev = hornActive;
        }

        bool bellActive = isLoco && bell_button.rk.state;
        if (bellActive != s_bellPrev) {
            s_engine->triggerBell(bellActive);
            s_bellPrev = bellActive;
        }

        // ── Steering Auto Turn Signals ──
        if (steerVal > 35)       s_autoTurnRight = true;
        else if (steerVal < 10)  s_autoTurnRight = false;

        if (steerVal < -35)      s_autoTurnLeft = true;
        else if (steerVal > -10) s_autoTurnLeft = false;

        // Manual indicator buttons (left_indicator / right_indicator) merge with the
        // steering auto signals: either source keeps the indicator active, and the
        // manual button (or steering center) releases its own side. Buttons are truck
        // widgets, so gate them on the vehicle type (auto signals are already inert on
        // a loco since steerVal is pinned to 0).
        bool turnSignalL = (!isLoco && left_indicator.rk.state) || s_autoTurnLeft;
        bool turnSignalR = (!isLoco && right_indicator.rk.state) || s_autoTurnRight;

        // ── Dynamic Deceleration Brake Light ──
        int16_t drop = s_prevThrottlePct - throttlePct;
        if (drop > 30) {
            s_decelBrakeTime = millis() + 1500;
        }
        bool decelBrakeActive = (millis() < s_decelBrakeTime);
        s_prevThrottlePct = throttlePct;

        // ── Headlight 3-State Stepping (Bit 0 / Item A) ──
        bool bit0State = (bits & 0x01);
        if (bit0State && !s_lastBit0State) {
            s_headlightMode = (s_headlightMode + 1) % 3; // 0=Off, 1=Low Beam (40%), 2=High Beam (100%)
        }
        s_lastBit0State = bit0State;

        // ── Apply Lights with Automation ──
        bool hazardActive = (bits & 0x08); // Bit 3 / Item D triggers manual Hazards
        bool hazardFlash = hazardActive && ((millis() / 333) % 2 == 0);

        applyLightsWithAutomation(bits,
                                 hazardActive ? hazardFlash : turnSignalL,
                                 hazardActive ? hazardFlash : turnSignalR,
                                 decelBrakeActive,
                                 brakePressed,
                                 s_headlightMode,
                                 !isLoco && reverse);   // gear R auto-lights the reversing lamp

        // ── Indicator Click Sound ──
        bool indicatorActive = hazardActive || turnSignalL || turnSignalR;
        if (indicatorActive != s_indicatorPrev) {
            s_engine->triggerIndicator(indicatorActive);
            s_indicatorPrev = indicatorActive;
        }

        // ── Telemetry ──
        uint32_t now = millis();
        if (now - s_lastTelemetry >= 500) {
            s_lastTelemetry = now;
            updateTelemetry(motorSpeed, batV);
        }
    }

private:
    static HardwareConfig* s_hw;
    static RcEngineSound*  s_engine;
    static VehicleProfile* s_profile;

    static bool     s_brakePrev;
    static bool     s_hornPrev;
    static bool     s_bellPrev;
    static bool     s_reversePrev;
    static uint32_t s_lastTelemetry;
    static char     s_battBuf[8];
    static char     s_speedBuf[8];

    // Battery safety
    static uint8_t  s_cellCount;
    static float    s_cutoffVoltage;
    static uint32_t s_lowVoltageStart;
    static bool     s_batteryCutoff;

    // Lighting automation
    static int16_t  s_prevThrottlePct;
    static uint32_t s_decelBrakeTime;
    static uint8_t  s_headlightMode;
    static bool     s_lastBit0State;
    static bool     s_autoTurnLeft;
    static bool     s_autoTurnRight;
    static bool     s_jakeBrakePrev;
    static bool     s_indicatorPrev;
    static uint8_t  s_gearPrev;
    static bool     s_parkingBrakePrev;

    static void applyLightsWithAutomation(uint8_t bits, bool turnL, bool turnR, bool decelBrake, bool manualBrake, uint8_t headlightMode, bool autoReverseLight) {
        if (!s_hw) return;
        const HardwareConfig::Lights& L = s_hw->lights;
        bool manualHead = (bits & 0x01);
        bool manualTail = (bits & 0x02);
        bool brakeActive = (bits & 0x04) || manualBrake || decelBrake;
        bool manualRev  = (bits & 0x10) || autoReverseLight; // Item E manual override OR gear-R automatic

        uint8_t headBright = 0;
        if (headlightMode == 1)      headBright = (uint8_t)(L.headLight.brightness * 0.40f);
        else if (headlightMode == 2 || manualHead) headBright = L.headLight.brightness;

        HardwareInit::setLight(L.headLight.pin,       headBright);
        HardwareInit::setLight(L.tailLight.pin,       manualTail ? L.tailLight.brightness : (headBright > 0 ? (uint8_t)(L.tailLight.brightness * 0.30f) : 0));
        HardwareInit::setLight(L.brakeLight.pin,      brakeActive ? L.brakeLight.brightness : 0);
        HardwareInit::setLight(L.turnLight.leftPin,   turnL ? L.turnLight.brightness : 0);
        HardwareInit::setLight(L.turnLight.rightPin,  turnR ? L.turnLight.brightness : 0);
        HardwareInit::setLight(L.reversingLight.pin,  manualRev ? L.reversingLight.brightness : 0);
    }

    static void updateTelemetry(int16_t motorSpeed, float batV) {
        // Percent uses the config'd per-cell voltages (defaults 3.3V / 4.2V).
        const float cutoffPerCell = s_hw ? s_hw->battery.cutoffVoltage : 3.3f;
        const float fullPerCell   = s_hw ? s_hw->battery.fullVoltage   : 4.2f;
        int pct = (int)((batV - cutoffPerCell * s_cellCount) / ((fullPerCell - cutoffPerCell) * s_cellCount) * 100.0f);
        pct = constrain(pct, 0, 100);
        snprintf(s_battBuf, sizeof(s_battBuf), "%d", pct);
        telemetry_Battery.rk.content = s_battBuf;

        snprintf(s_speedBuf, sizeof(s_speedBuf), "%d", abs(motorSpeed));
        telemetry_Speed.rk.content = s_speedBuf;
    }
};

// Static member instantiations
HardwareConfig* VehicleController::s_hw = nullptr;
RcEngineSound*  VehicleController::s_engine = nullptr;
VehicleProfile* VehicleController::s_profile = nullptr;

bool     VehicleController::s_brakePrev = false;
bool     VehicleController::s_hornPrev = false;
bool     VehicleController::s_bellPrev = false;
bool     VehicleController::s_reversePrev = false;
uint32_t VehicleController::s_lastTelemetry = 0;
char     VehicleController::s_battBuf[8] = "--";
char     VehicleController::s_speedBuf[8] = "--";

uint8_t  VehicleController::s_cellCount = 2;
float    VehicleController::s_cutoffVoltage = 6.6f;
uint32_t VehicleController::s_lowVoltageStart = 0;
bool     VehicleController::s_batteryCutoff = false;

int16_t  VehicleController::s_prevThrottlePct = 0;
uint32_t VehicleController::s_decelBrakeTime = 0;
uint8_t  VehicleController::s_headlightMode = 0;
bool     VehicleController::s_lastBit0State = false;
bool     VehicleController::s_autoTurnLeft = false;
bool     VehicleController::s_autoTurnRight = false;
bool     VehicleController::s_jakeBrakePrev = false;
bool     VehicleController::s_indicatorPrev = false;
uint8_t  VehicleController::s_gearPrev = 1;
bool     VehicleController::s_parkingBrakePrev = false;

int16_t  VehicleController::aux_hydraulic1 = 0;
int16_t  VehicleController::aux_hydraulic2 = 0;
bool     VehicleController::bucket_rattle_trigger = false;
bool     VehicleController::dump_bed_toggle = false;
