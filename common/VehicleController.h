#pragma once

#include <Arduino.h>
#include "RADIOKIT.h"
#include "UiLogger.h"
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
    static bool    bucket_rattle_trigger;
    static bool    dump_bed_toggle;

    static const char* engineStateStr(RcEngineSound::EngineState s) {
        switch (s) {
            case RcEngineSound::OFF: return "OFF";
            case RcEngineSound::STARTING: return "STARTING";
            case RcEngineSound::RUNNING: return "RUNNING";
            case RcEngineSound::STOPPING: return "STOPPING";
            case RcEngineSound::PARKING_BRAKE: return "PARKING_BRAKE";
            default: return "UNKNOWN";
        }
    }

    static void applyConfiguredLightMask(const HardwareConfig::Lights& L, bool auxHwConfigured) {
        uint8_t truckMask = HardwareInit::getConfiguredLightMask(L, false);
        uint8_t locoMask  = HardwareInit::getConfiguredLightMask(L, true);

        truck_light.setItemMask(truckMask);
        loco_light.setItemMask(locoMask);

        Serial.printf("[Lights] Configured Mask: Truck=0x%02X, Loco=0x%02X\n", truckMask, locoMask);
    }

    static void init(HardwareConfig* hw, RcEngineSound* engine, VehicleProfile* profile) {
        s_hw = hw;
        s_engine = engine;
        s_profile = profile;

        if (s_hw) {
            applyConfiguredLightMask(s_hw->lights, s_hw->auxLight.configured);
        }

        // Master volume comes from the hardware config
        if (s_hw && s_profile) {
            s_profile->config.sound.master = s_hw->sound.volume;
        }

        s_engineStatePrev = RcEngineSound::OFF;
        s_brakePrev = false;
        s_hornPrev = false;
        s_bellPrev = false;
        s_reversePrev = false;
        s_lastTelemetry = 0;
        s_prevThrottlePct = 0;
        s_decelBrakeTime = 0;
        s_headlightMode = 0;
        s_lastHeadBright = 0;
        s_lastFullBright = 0;
        s_fogLampPrev = false;
        s_autoTurnLeft = false;
        s_autoTurnRight = false;
        s_leftTurnArmed = false;
        s_rightTurnArmed = false;
        s_leftTurnBaseline = 0;
        s_leftTurnPeak = 0;
        s_rightTurnBaseline = 0;
        s_rightTurnPeak = 0;
        s_leftIndPrev = false;
        s_rightIndPrev = false;
        s_leftIndActive = false;
        s_rightIndActive = false;
        s_leftIndSuppressed = false;
        s_rightIndSuppressed = false;
        s_engineStartTogglePrev = false;
        s_cutoffLightResetDone = false;
        s_jakeBrakePrev = false;
        s_indicatorPrev = false;
        s_gearPrev = 1;             // default Park until the radio reports a selection
        s_parkingBrakePrev = false;
        aux_hydraulic1 = 0;
        bucket_rattle_trigger = false;
        dump_bed_toggle = false;

        strcpy(s_battBuf, "--");
        strcpy(s_speedBuf, "--");

        // Safe startup: center steering, stop motor, zero aux outputs
        HardwareInit::setServo(0);
        HardwareInit::setMotor(0);
        HardwareInit::setAuxMotor(0);
        HardwareInit::setAuxLight(0);

        // ── Battery Cell Count: config-driven, voltage auto-detect as fallback ──
        // Board hardware config is authoritative (cell_count: 1..4). Only when the
        // config omits it (cell_count: 0) do we fall back to legacy voltage-based
        // detection so the cutoff still engages for unknown packs.
        float sumV = 0;
        for (int i = 0; i < 10; ++i) {
            float pinV = analogReadMilliVolts(POWER::VOLTAGE_SENS) / 1000.0f;

            sumV += pinV * s_hw->battery.vScale + s_hw->battery.vOffset;
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

        s_warningVoltage = s_cellCount * s_hw->battery.warningVoltage;
        s_cutoffVoltage = s_cellCount * s_hw->battery.cutoffVoltage;
        s_filteredBatV = 0.0f;
        s_lowVoltageStart = 0;
        s_batteryWarning = false;
        s_batteryCutoff = false;
        s_disconnectStart = millis();
        s_inWarningPhase = false;

        Serial.printf("[VehicleController] Battery warning: %.2fV (%.2fV/cell), cutoff: %.2fV (%.2fV/cell)\n",
                      s_warningVoltage, s_hw->battery.warningVoltage,
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

    static bool isBatteryWarning() { return s_batteryWarning; }
    static bool isBatteryCutoff() { return s_batteryCutoff; }
    static bool isDisconnectWarning() { return s_inWarningPhase; }
    static bool isChargingState() { return HardwareInit::isCharging(); }

    static void update() {
        if (!s_hw || !s_engine || !s_profile) return;

        // ── Power Button Click Handler ──
        if (HardwareInit::consumeButtonClicked()) {
            s_disconnectStart = millis();
            s_inWarningPhase = false;
        }

        // ── 3-State Board Power & Disconnect Auto Power-Off ──
        bool isCharging = HardwareInit::isCharging();
        if (isCharging && !RadioKit.isConnected()) {
            s_disconnectStart = 0;
            s_inWarningPhase = false;
            HardwareInit::setAllMotors(0);

            // Drive charging indicator (configured pin or headlight fallback)
            uint8_t chgPin = s_hw->charging.configured ? s_hw->charging.pin : s_hw->lights.headLight.pin;
            if (chgPin != 0xFF) {
                if (s_hw->charging.mode == 1) {
                    HardwareInit::setLightBlink(chgPin, true, 500, 500, 100);
                } else if (s_hw->charging.mode == 2) {
                    uint8_t pulseDuty = (uint8_t)(50.0f + 50.0f * sinf(millis() * 0.005f));
                    HardwareInit::setLight(chgPin, pulseDuty);
                } else {
                    HardwareInit::setLight(chgPin, 100);
                }
            }
        } else if (RadioKit.isConnected()) {
            s_disconnectStart = 0;
            s_inWarningPhase = false;
            if (!HardwareInit::isPowerLatched()) {
                HardwareInit::latchPower();
            }
        } else {
            // Disconnected state
            uint32_t timeoutMs = (uint32_t)s_hw->power.disconnectTimeoutS * 1000U;
            uint32_t warningMs = (uint32_t)s_hw->power.warningWindowS * 1000U;
            if (timeoutMs > 0) {
                if (s_disconnectStart == 0) {
                    s_disconnectStart = millis();
                }
                uint32_t elapsed = millis() - s_disconnectStart;
                if (elapsed >= timeoutMs) {
                    Serial.println("[VehicleController] Disconnect timeout reached -> powerOff()");
                    HardwareInit::powerOff();
                    return;
                } else if (warningMs > 0 && elapsed >= (timeoutMs - warningMs)) {
                    s_inWarningPhase = true;
                    if (s_hw->power.indicatorPin != 0xFF) {
                        HardwareInit::setLightBlink(s_hw->power.indicatorPin, true, 500, 500, 100);
                    }
                } else {
                    s_inWarningPhase = false;
                }
            }
        }

        // Vehicle type from vehicle-config.json is the single source of truth:
        // TRUCK and LOCOMOTIVE widget sets are mutually exclusive, never active together.
        // Dispatch on the canonical enum: LOCOMOTIVE -> loco widget set; TRUCK, EXCAVATOR
        // (recognized stub, see init) and UNKNOWN all use the truck widget set.
        const RcEngineSound::VehicleType vtype = s_profile->config.type;
        const bool isLoco = (vtype == RcEngineSound::VEHICLE_LOCOMOTIVE);

        // ── Battery Protection & Low Voltage Cutoff ──
        float pinV = analogReadMilliVolts(POWER::VOLTAGE_SENS) / 1000.0f;
        float rawBatV = pinV * s_hw->battery.vScale + s_hw->battery.vOffset;

        // Exponential moving average filter (EMA) to reject motor PWM / inrush noise
        if (s_filteredBatV <= 0.1f) {
            s_filteredBatV = rawBatV;
        } else {
            s_filteredBatV = 0.90f * s_filteredBatV + 0.10f * rawBatV;
        }
        float batV = s_filteredBatV;

        if (batV < s_warningVoltage) {
            if (!s_batteryWarning) {
                UiLogger::logf("WARN: Low battery (%.2fV)", batV);
            }
            s_batteryWarning = true;
        } else if (batV > (s_warningVoltage + 0.2f * s_cellCount)) {
            s_batteryWarning = false;
        }

        uint32_t cutoffDelayMs = (uint32_t)(s_hw->power.cutoffDelayS * 1000.0f);
        if (batV < s_cutoffVoltage) {
            if (s_lowVoltageStart == 0) s_lowVoltageStart = millis();
            else if (millis() - s_lowVoltageStart >= cutoffDelayMs) {
                s_batteryCutoff = true;
            }
        } else if (batV > (s_cutoffVoltage + 0.2f * s_cellCount)) {
            s_lowVoltageStart = 0;
            s_batteryCutoff = false;
        }

        if (s_batteryCutoff) {
            HardwareInit::setAllMotors(0);
            s_engine->triggerOutOfFuel(true);
            // Once on entering cutoff, cancel every LED animation so a hazard or
            // turn blink can never strand an LED on; the fade-out below then runs
            // uninterrupted. Skipped on later iterations to preserve the fade.
            if (!s_cutoffLightResetDone) {
                s_cutoffLightResetDone = true;
                UiLogger::logf("CRITICAL: Battery cutoff (%.2fV)", batV);
                HardwareInit::stopLightAnimations();
            }
            // Minimal alarm state: zeroed bits (non-blinking) + out-of-fuel sound.
            applyLightsWithAutomation(0, false, false, false, false, 0, false, false, isLoco);
            updateTelemetry(0, 0, 0, 1, false, false, false, 0, batV);
            HardwareInit::powerOff();
            return;
        } else {
            s_cutoffLightResetDone = false;
            s_engine->triggerOutOfFuel(false);
        }

        // ── Throttle input ──
        int16_t throttlePct = 0;
        if (isLoco) {
            throttlePct = constrain(throttle_slider.rk.value, 0, 100);
        } else {
            // gas_pedal operates from -100 (idle/released) to +100 (full throttle)
            int16_t rawPedal = constrain(gas_pedal.rk.value, -100, 100);
            throttlePct = (rawPedal + 100) / 2;
        }

        // ── Engine Start / Power State Machine ──
        // Engine power toggles are type-driven and exclusive: start_button (TRUCK)
        // or engine_button (LOCOMOTIVE). Latched toggles: ON = run, OFF = stop.
        RcEngineSound::EngineState eState = s_engine->getState();
        if (eState != s_engineStatePrev) {
            Serial.printf("[EVENT] EngineState -> %s (RPM: %u)\n", engineStateStr(eState), s_engine->getRpm());
            s_engineStatePrev = eState;
        }
        uint8_t bits = isLoco ? loco_light.rk.value : truck_light.rk.value;
        bool engineStartToggle = isLoco ? engine_button.rk.state : start_button.rk.state;

        // Auto Gear Shift Interlock on Engine Start / Stop (TRUCK)
        if (!isLoco) {
            if (engineStartToggle && !s_engineStartTogglePrev) {
                // Engine Start edge: auto-shift to Drive (D=0)
                gear_switch.rk.value = 0;
            } else if (!engineStartToggle && s_engineStartTogglePrev) {
                // Engine Stop edge: auto-shift to Park (P=1)
                gear_switch.rk.value = 1;
            } else if (!engineStartToggle && gear_switch.rk.value != 1) {
                // While engine is OFF/stopped, force Park (P=1)
                gear_switch.rk.value = 1;
            }
        }
        s_engineStartTogglePrev = engineStartToggle;

        if (eState == RcEngineSound::OFF) {
            if (engineStartToggle) {  // Strict per spec: the Engine Power toggle is the sole start trigger
                s_engine->startEngine();
            }
            HardwareInit::setAllMotors(0);
            throttlePct = 0;
        } else if (eState == RcEngineSound::STARTING) {
            if (!engineStartToggle) s_engine->stopEngine();  // toggled OFF mid-crank cancels the start
            HardwareInit::setAllMotors(0);
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
                Serial.printf("[EVENT] Gear -> %s\n", gear == 0 ? "D" : gear == 1 ? "P" : "R");
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

        // Proportional brake blend: brake_pedal normalized from [-100, 100] to [0, 100]%
        int16_t brakePct = 0;
        if (!isLoco) {
            int16_t rawBrake = constrain(brake_pedal.rk.value, -100, 100);
            brakePct = (rawBrake + 100) / 2;
        }
        int16_t motorThrottle = throttlePct;
        if (parkingBrake) {
            motorThrottle = 0;   // Park locks the motor regardless of throttle or brake
        } else if (brakePct > 20) {
            // Renormalize over the active range so the ramp is continuous at the deadband
            // edge (scale 1.0 at brakePct=20, 0.0 at full brake 100).
            motorThrottle = (int16_t)((int32_t)throttlePct * (100 - brakePct) / 80);
        }
        int16_t motorSpeed = reverse ? -motorThrottle : motorThrottle;

        if (s_hw->drivetrainType == HardwareConfig::SKID_STEER) {
            if (eState == RcEngineSound::RUNNING) {
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
                HardwareInit::setSkidMotors(0, 0);
            }
        } else {
            // Ackermann steering: motor drives only while RUNNING and not in Park
            if (eState == RcEngineSound::RUNNING && !parkingBrake) {
                HardwareInit::setMotor(motorSpeed);
            } else {
                HardwareInit::setMotor(0);
            }
            // Steer-by-wire: servo continuously tracks steering_wheel in all active states
            HardwareInit::setServo(steerVal);
        }

        // ── Work Machine Aux Outputs ──
        // Truck page aux_slider drives the configured aux motor channel (mixer:
        // proportional incl. direction; tipper: momentary since the slider is
        // self-centering) + hydraulic flow sound + load governor. Loco page has
        // no aux control. trailer_dcc configs never get here: the parser leaves
        // the channel unconfigured, so setAuxMotor() is a no-op.
        aux_hydraulic1 = isLoco ? 0 : aux_slider.rk.value;
        HardwareInit::setAuxMotor(aux_hydraulic1);

        // Aux light (work lamp) follows aux activity: on while the aux channel is
        // active (slider or dump bed), off when idle.
        bool auxActive = (abs(aux_hydraulic1) > 10 || dump_bed_toggle);
        if (s_hw->auxLight.configured) {
            HardwareInit::setAuxLight(auxActive ? s_hw->auxLight.brightness : 0);
        }

        // ── Work Machine Sound FX & Engine Pump Load Governor ──
        bool hydraulicFlowActive = auxActive;
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
            Serial.printf("[EVENT] JakeBrake -> %s (RPM: %u)\n", jakeCondition ? "ON" : "OFF", curRpm);
            s_jakeBrakePrev = jakeCondition;
        }

        // ── Brake pedal ──
        bool brakePressed = brakePct > 20;
        if (brakePressed != s_brakePrev) {
            s_engine->triggerBrake(brakePressed);
            s_brakePrev = brakePressed;
        }

        // ── Horn (TRUCK) / Bell (LOCOMOTIVE) ──
        // Type-driven: horn_button lives on the truck page, bell_button on the loco page.
        bool hornActive = !isLoco && horn_button.rk.state;
        if (hornActive != s_hornPrev) {
            s_engine->triggerHorn(hornActive);
            Serial.printf("[EVENT] Horn -> %s\n", hornActive ? "ON" : "OFF");
            s_hornPrev = hornActive;
        }

        bool bellActive = isLoco && bell_button.rk.state;
        if (bellActive != s_bellPrev) {
            s_engine->triggerBell(bellActive);
            Serial.printf("[EVENT] Bell -> %s\n", bellActive ? "ON" : "OFF");
            s_bellPrev = bellActive;
        }

        // ── Real Vehicle Turn Indicator Auto-Cancellation (Relative to baseline at press) ──
        if (!isLoco) {
            bool rawLeft = left_indicator.rk.state;
            bool rawRight = right_indicator.rk.state;

            // Clear suppression once the app releases/resets the toggle to false
            if (!rawLeft) s_leftIndSuppressed = false;
            if (!rawRight) s_rightIndSuppressed = false;

            // Effective state accounts for firmware suppression
            bool effLeft = rawLeft && !s_leftIndSuppressed;
            bool effRight = rawRight && !s_rightIndSuppressed;

            // Activation edges
            bool leftEdge = effLeft && !s_leftIndActive;
            bool rightEdge = effRight && !s_rightIndActive;

            if (rightEdge) {
                if (s_leftIndActive || rawLeft) s_leftIndSuppressed = true;
                s_leftIndActive = false;
                s_leftTurnArmed = false;
                s_rightTurnArmed = false;
                s_rightTurnBaseline = steerVal;
                s_rightTurnPeak = steerVal;
                s_rightIndActive = true;
                Serial.printf("[EVENT] Right indicator ON (Baseline: %d)\n", s_rightTurnBaseline);
            } else if (leftEdge) {
                if (s_rightIndActive || rawRight) s_rightIndSuppressed = true;
                s_rightIndActive = false;
                s_rightTurnArmed = false;
                s_leftTurnArmed = false;
                s_leftTurnBaseline = steerVal;
                s_leftTurnPeak = steerVal;
                s_leftIndActive = true;
                Serial.printf("[EVENT] Left indicator ON (Baseline: %d)\n", s_leftTurnBaseline);
            } else {
                if (!effLeft) {
                    s_leftIndActive = false;
                    s_leftTurnArmed = false;
                }
                if (!effRight) {
                    s_rightIndActive = false;
                    s_rightTurnArmed = false;
                }
            }

            // Left turn indicator cancel logic
            if (s_leftIndActive) {
                int16_t delta = steerVal - s_leftTurnBaseline;
                if (delta > 15 && steerVal > 10) {
                    // Cancel immediately if driver steers opposite (right) by >15% and past center
                    s_leftIndActive = false;
                    s_leftIndSuppressed = true;
                    s_leftTurnArmed = false;
                    Serial.println("[EVENT] Left indicator auto-cancelled (opposite steering)");
                } else if (delta <= -20) {
                    s_leftTurnArmed = true; // Armed when steering into left turn >=20% from baseline
                    if (steerVal < s_leftTurnPeak) s_leftTurnPeak = steerVal;
                } else if (s_leftTurnArmed && (steerVal >= s_leftTurnPeak + 15 || steerVal >= -5)) {
                    // Cancel when wheel returns towards center/baseline with hysteresis
                    s_leftIndActive = false;
                    s_leftIndSuppressed = true;
                    s_leftTurnArmed = false;
                    Serial.println("[EVENT] Left indicator auto-cancelled (wheel returned to center)");
                }
            }

            // Right turn indicator cancel logic
            if (s_rightIndActive) {
                int16_t delta = steerVal - s_rightTurnBaseline;
                if (delta < -15 && steerVal < -10) {
                    // Cancel immediately if driver steers opposite (left) by >15% and past center
                    s_rightIndActive = false;
                    s_rightIndSuppressed = true;
                    s_rightTurnArmed = false;
                    Serial.println("[EVENT] Right indicator auto-cancelled (opposite steering)");
                } else if (delta >= 20) {
                    s_rightTurnArmed = true; // Armed when steering into right turn >=20% from baseline
                    if (steerVal > s_rightTurnPeak) s_rightTurnPeak = steerVal;
                } else if (s_rightTurnArmed && (steerVal <= s_rightTurnPeak - 15 || steerVal <= 5)) {
                    // Cancel when wheel returns towards center/baseline with hysteresis
                    s_rightIndActive = false;
                    s_rightIndSuppressed = true;
                    s_rightTurnArmed = false;
                    Serial.println("[EVENT] Right indicator auto-cancelled (wheel returned to center)");
                }
            }

            left_indicator.rk.state = s_leftIndActive;
            right_indicator.rk.state = s_rightIndActive;
            s_leftIndPrev = rawLeft;
            s_rightIndPrev = rawRight;
        }

        bool turnSignalL = !isLoco && s_leftIndActive;
        bool turnSignalR = !isLoco && s_rightIndActive;

        // ── Dynamic Deceleration Brake Light ──
        int16_t drop = s_prevThrottlePct - throttlePct;
        if (drop > 30) {
            s_decelBrakeTime = millis() + 1500;
        }
        bool decelBrakeActive = (millis() < s_decelBrakeTime);
        s_prevThrottlePct = throttlePct;

        // ── Dedicated Headlight, High Beam & Fog Lamp Control ──
        uint8_t headlightMode = 0; // 0=Off, 1=Head Light (40%), 2=High Beam (100%)
        bool fogLamp = false;
        if (!isLoco) {
            bool headLight = (bits & 0x01); // Item 0: Head Light
            bool highBeam  = (bits & 0x02); // Item 1: High Beam
            fogLamp        = (bits & 0x04); // Item 2: Fog Lamp

            // High beam is coupled to headlight:
            // 1. High beam can only be ON if headlight is ON.
            // 2. If headlight is switched OFF, high beam is also forced OFF and cleared in UI.
            if (!headLight && (truck_light.rk.value & 0x02)) {
                truck_light.rk.value &= ~0x02;
                highBeam = false;
            }

            if (headLight) {
                headlightMode = highBeam ? 2 : 1;
            } else {
                headlightMode = 0;
            }

            if (headlightMode != s_headlightMode) {
                s_headlightMode = headlightMode;
                Serial.printf("[EVENT] Headlight -> %s\n",
                              s_headlightMode == 0 ? "OFF" : s_headlightMode == 1 ? "LOW" : "HIGH");
            }
            if (fogLamp != s_fogLampPrev) {
                s_fogLampPrev = fogLamp;
                Serial.printf("[EVENT] FogLamp -> %s\n", fogLamp ? "ON" : "OFF");
            }
        } else {
            // Locomotive headlight mapping from loco_light bits
            bool headLight = (bits & 0x01);
            bool highBeam  = (bits & 0x02);
            if (!headLight && (loco_light.rk.value & 0x02)) {
                loco_light.rk.value &= ~0x02;
                highBeam = false;
            }
            if (headLight) {
                headlightMode = highBeam ? 2 : 1;
            } else {
                headlightMode = 0;
            }
            s_headlightMode = headlightMode;
        }

        // ── Apply Lights with Automation ──
        // Hazard (Bit 3 / Item D) drives both indicators; the blink engine
        // handles the on/off timing from the config interval — no hand-rolled
        // flash here.
        bool hazardActive = (bits & 0x08) || s_inWarningPhase;
        applyLightsWithAutomation(bits,
                                 hazardActive || turnSignalL,
                                 hazardActive || turnSignalR,
                                 decelBrakeActive,
                                 brakePressed,
                                 headlightMode,
                                 !isLoco && reverse,
                                 fogLamp,
                                 isLoco);   // gear R auto-lights the reversing lamp

        // ── Indicator Click Sound ──
        bool indicatorActive = hazardActive || turnSignalL || turnSignalR;
        if (indicatorActive != s_indicatorPrev) {
            s_engine->triggerIndicator(indicatorActive);
            s_indicatorPrev = indicatorActive;
        }

        // ── Telemetry & Serial Debug Stream ──
        uint32_t now = millis();
        if (now - s_lastTelemetry >= 1000) {
            s_lastTelemetry = now;
            updateTelemetry(motorSpeed, steerVal, throttlePct, gear, brakePressed, turnSignalL, turnSignalR, bits, batV);
        }
    }

private:
    static HardwareConfig* s_hw;
    static RcEngineSound*  s_engine;
    static VehicleProfile* s_profile;

    static RcEngineSound::EngineState s_engineStatePrev;
    static bool     s_brakePrev;
    static bool     s_hornPrev;
    static bool     s_bellPrev;
    static bool     s_reversePrev;
    static uint32_t s_lastTelemetry;
    static char     s_battBuf[8];

    static char     s_speedBuf[8];

    // Battery safety
    static uint8_t  s_cellCount;
    static float    s_warningVoltage;
    static float    s_cutoffVoltage;
    static float    s_filteredBatV;
    static uint32_t s_lowVoltageStart;
    static bool     s_batteryWarning;
    static bool     s_batteryCutoff;
    static uint32_t s_disconnectStart;
    static bool     s_inWarningPhase;

    // Lighting automation
    static int16_t  s_prevThrottlePct;
    static uint32_t s_decelBrakeTime;
    static uint8_t  s_headlightMode;
    static uint8_t  s_lastHeadBright;   // last commanded headlight target (fade-on-change)
    static uint8_t  s_lastFullBright;   // last commanded full beam target (fade-on-change)
    static bool     s_fogLampPrev;
    static bool     s_autoTurnLeft;
    static bool     s_autoTurnRight;
    static bool     s_leftTurnArmed;
    static bool     s_rightTurnArmed;
    static int16_t  s_leftTurnBaseline;
    static int16_t  s_leftTurnPeak;
    static int16_t  s_rightTurnBaseline;
    static int16_t  s_rightTurnPeak;
    static bool     s_leftIndPrev;
    static bool     s_rightIndPrev;
    static bool     s_leftIndActive;
    static bool     s_rightIndActive;
    static bool     s_leftIndSuppressed;
    static bool     s_rightIndSuppressed;
    static bool     s_engineStartTogglePrev;
    static bool     s_cutoffLightResetDone;
    static bool     s_jakeBrakePrev;
    static bool     s_indicatorPrev;
    static uint8_t  s_gearPrev;
    static bool     s_parkingBrakePrev;

    static void applyLightsWithAutomation(uint8_t bits, bool turnL, bool turnR, bool decelBrake, bool manualBrake, uint8_t headlightMode, bool autoReverseLight, bool fogLamp, bool isLoco) {
        if (!s_hw) return;
        const HardwareConfig::Lights& L = s_hw->lights;
        bool manualTail = isLoco ? (bits & 0x02) : false;
        bool brakeActive = manualBrake || decelBrake;
        bool manualRev  = autoReverseLight; // gear-R automatic

        uint8_t headBright = 0;
        uint8_t fullBright = 0;
        if (headlightMode == 1) {
            headBright = (uint8_t)(L.headLight.brightness * 0.40f);
            fullBright = 0;
        } else if (headlightMode == 2) {
            if (L.fullBeam.configured) {
                headBright = (uint8_t)(L.headLight.brightness * 0.40f);
                fullBright = L.fullBeam.brightness;
            } else {
                headBright = L.headLight.brightness;
                fullBright = 0;
            }
        }

        // Headlight & full beam steps transition via a fade, triggered only on a target
        // change so the animation engine ramps instead of snapping.
        if (headBright != s_lastHeadBright) {
            s_lastHeadBright = headBright;
            HardwareInit::setLightFade(L.headLight.pin, headBright, s_hw->animation.fadeDurationMs);
        }
        if (L.fullBeam.configured && fullBright != s_lastFullBright) {
            s_lastFullBright = fullBright;
            HardwareInit::setLightFade(L.fullBeam.pin, fullBright, s_hw->animation.fadeDurationMs);
        }

        // Tail tracks the headlight's LIVE duty so it follows the fade naturally
        // (instead of snapping to the final target mid-ramp).
        uint8_t headLive = HardwareInit::getLightDutyPercent(L.headLight.pin);
        uint8_t tailBright = manualTail ? L.tailLight.brightness
                                       : (uint8_t)(headLive * 0.30f);

        uint8_t brakeBright = brakeActive ? L.brakeLight.brightness : 0;
        uint8_t revBright   = manualRev ? L.reversingLight.brightness : 0;

        // Merge shared/aliased pins to prevent any flickering between independent writes
        if (L.reversingLight.pin != 0xFF && L.reversingLight.pin == L.brakeLight.pin) {
            brakeBright = max(brakeBright, revBright);
            revBright = 0;
        }
        if (L.tailLight.pin != 0xFF && L.tailLight.pin == L.brakeLight.pin) {
            brakeBright = max(brakeBright, tailBright);
            tailBright = 0;
        }

        HardwareInit::setLight(L.tailLight.pin,      tailBright);
        HardwareInit::setLight(L.brakeLight.pin,     brakeBright);
        if (L.reversingLight.pin != 0xFF && L.reversingLight.pin != L.brakeLight.pin) {
            HardwareInit::setLight(L.reversingLight.pin, revBright);
        }

        // Bit 2: Fog Lamp
        uint8_t fogDuty = (bits & 0x04) ? L.fogLamp.brightness : 0;
        if (L.fogLamp.configured) {
            HardwareInit::setLight(L.fogLamp.pin, fogDuty);
        }

        // Bit 3: Hazard Light (Truck) or Ditch Lights (Locomotive)
        if (isLoco) {
            bool ditchOn = (bits & 0x08);
            HardwareInit::setDitchLights(ditchOn, L.ditchLight.intervalMs);
        }

        // Bit 4: Beacon Light (Strobe / Flasher)
        bool beaconOn = (bits & 0x10);
        HardwareInit::setBeacon(beaconOn);

        // Bit 5: Cab Light
        uint8_t cabDuty = (bits & 0x20) ? L.cabLight.brightness : 0;
        if (L.cabLight.configured) {
            HardwareInit::setLight(L.cabLight.pin, cabDuty);
        }

        // Bit 6: Work Light (Truck) or Step Light (Locomotive)
        if (!isLoco) {
            uint8_t workDuty = (bits & 0x40) ? L.workLight.brightness : 0;
            if (L.workLight.configured) {
                HardwareInit::setLight(L.workLight.pin, workDuty);
            }
        } else {
            uint8_t stepDuty = (bits & 0x40) ? L.stepLight.brightness : 0;
            if (L.stepLight.configured) {
                HardwareInit::setLight(L.stepLight.pin, stepDuty);
            }
        }

        // Bit 7: Aux Light
        uint8_t auxDuty = (bits & 0x80) ? L.auxLight.brightness : 0;
        if (L.auxLight.configured) {
            HardwareInit::setLight(L.auxLight.pin, auxDuty);
        } else if (s_hw->auxLight.configured) {
            HardwareInit::setAuxLight((bits & 0x80) ? s_hw->auxLight.brightness : 0);
        }

        // Turn signals / hazards run through the blink engine with the config
        // interval/duty. The blink engine owns the duty while active — no
        // static setLight() may target these pins during a blink.
        HardwareInit::setLightBlink(L.turnLight.leftPin,  turnL, L.turnLight.intervalOn, L.turnLight.intervalOff, L.turnLight.brightness);
        HardwareInit::setLightBlink(L.turnLight.rightPin, turnR, L.turnLight.intervalOn, L.turnLight.intervalOff, L.turnLight.brightness);
    }

    static void updateTelemetry(int16_t motorSpeed, int16_t steerVal, int16_t throttlePct, uint8_t gear, bool brakePressed, bool turnL, bool turnR, uint8_t bits, float batV) {
        // Battery percentage uses warning voltage as 0% floor and full voltage as 100%
        const float warnPerCell = s_hw ? s_hw->battery.warningVoltage : 3.5f;
        const float fullPerCell = s_hw ? s_hw->battery.fullVoltage    : 4.2f;
        const float minV = warnPerCell * s_cellCount;
        const float maxV = fullPerCell * s_cellCount;
        int pct = 0;
        if (maxV > minV && batV > minV) {
            pct = (int)(((batV - minV) / (maxV - minV)) * 100.0f + 0.5f);
        }
        pct = constrain(pct, 0, 100);
        snprintf(s_battBuf, sizeof(s_battBuf), "%d", pct);
        telemetry_Battery.rk.content = s_battBuf;

        // Speed scaled to 0-200 km/h based on instantaneous motor speed
        int speedKmph = abs(motorSpeed) * 2;
        snprintf(s_speedBuf, sizeof(s_speedBuf), "%d", speedKmph);
        telemetry_Speed.rk.content = s_speedBuf;

        // Structured Serial Telemetry for USB Monitoring & Host Automation
        const char* gearStr = (gear == 0) ? "D" : (gear == 2) ? "R" : "P";
        const char* eStateStr = s_engine ? engineStateStr(s_engine->getState()) : "OFF";
        uint16_t rpm = s_engine ? s_engine->getRpm() : 0;
        Serial.printf("[STATUS] Eng:%s RPM:%u Thr:%d%% Mot:%d%% Spd:%dkm/h Steer:%d Gear:%s Brk:%d Head:%d L:%d R:%d Bat:%.2fV (%s%%)\n",
                      eStateStr, rpm, throttlePct, motorSpeed, speedKmph, steerVal, gearStr,
                      brakePressed ? 1 : 0, s_headlightMode, turnL ? 1 : 0, turnR ? 1 : 0,
                      batV, s_battBuf);
    }
};

// Static member instantiations
HardwareConfig* VehicleController::s_hw = nullptr;
RcEngineSound*  VehicleController::s_engine = nullptr;
VehicleProfile* VehicleController::s_profile = nullptr;

RcEngineSound::EngineState VehicleController::s_engineStatePrev = RcEngineSound::OFF;
bool     VehicleController::s_brakePrev = false;
bool     VehicleController::s_hornPrev = false;
bool     VehicleController::s_bellPrev = false;
bool     VehicleController::s_reversePrev = false;
uint32_t VehicleController::s_lastTelemetry = 0;
char     VehicleController::s_battBuf[8] = "--";

char     VehicleController::s_speedBuf[8] = "--";

uint8_t  VehicleController::s_cellCount = 2;
float    VehicleController::s_warningVoltage = 7.0f;
float    VehicleController::s_cutoffVoltage = 6.6f;
float    VehicleController::s_filteredBatV = 0.0f;
uint32_t VehicleController::s_lowVoltageStart = 0;
bool     VehicleController::s_batteryWarning = false;
bool     VehicleController::s_batteryCutoff = false;
uint32_t VehicleController::s_disconnectStart = 0;
bool     VehicleController::s_inWarningPhase = false;

int16_t  VehicleController::s_prevThrottlePct = 0;
uint32_t VehicleController::s_decelBrakeTime = 0;
uint8_t  VehicleController::s_headlightMode = 0;
uint8_t  VehicleController::s_lastHeadBright = 0;
uint8_t  VehicleController::s_lastFullBright = 0;
bool     VehicleController::s_fogLampPrev = false;
bool     VehicleController::s_autoTurnLeft = false;
bool     VehicleController::s_autoTurnRight = false;
bool     VehicleController::s_leftTurnArmed = false;
bool     VehicleController::s_rightTurnArmed = false;
int16_t  VehicleController::s_leftTurnBaseline = 0;
int16_t  VehicleController::s_leftTurnPeak = 0;
int16_t  VehicleController::s_rightTurnBaseline = 0;
int16_t  VehicleController::s_rightTurnPeak = 0;
bool     VehicleController::s_leftIndPrev = false;
bool     VehicleController::s_rightIndPrev = false;
bool     VehicleController::s_leftIndActive = false;
bool     VehicleController::s_rightIndActive = false;
bool     VehicleController::s_leftIndSuppressed = false;
bool     VehicleController::s_rightIndSuppressed = false;
bool     VehicleController::s_engineStartTogglePrev = false;
bool     VehicleController::s_cutoffLightResetDone = false;
bool     VehicleController::s_jakeBrakePrev = false;
bool     VehicleController::s_indicatorPrev = false;
uint8_t  VehicleController::s_gearPrev = 1;
bool     VehicleController::s_parkingBrakePrev = false;

int16_t  VehicleController::aux_hydraulic1 = 0;
bool     VehicleController::bucket_rattle_trigger = false;
bool     VehicleController::dump_bed_toggle = false;
