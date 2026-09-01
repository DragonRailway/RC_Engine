#include "VehicleController.h"

// ── Static member definitions ──

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
HardwareInit::TurnMode VehicleController::s_turnModePrev = HardwareInit::TurnMode::OFF;
uint32_t VehicleController::s_lastIndicatorClick = 0;
uint8_t  VehicleController::s_gearPrev = 1;
bool     VehicleController::s_parkingBrakePrev = false;
bool     VehicleController::s_wasConnected = false;
bool     VehicleController::s_reconnectThrottleInterlock = false;
bool     VehicleController::s_disconnectEngineStopDone = false;

float    VehicleController::s_currentMotorSpeed = 0.0f;
uint32_t VehicleController::s_lastInertiaTime = 0;
bool     VehicleController::s_prevMotorMoving = false;

float    VehicleController::s_currentSteerAngle = 0.0f;
int8_t   VehicleController::s_lastSteerInputVal = 0;
uint32_t VehicleController::s_lastSteerTouchMs = 0;
uint32_t VehicleController::s_lastSteerPhysicsMs = 0;

bool     VehicleController::s_locoInitialized = false;
bool     VehicleController::s_activeDirection = true;
bool     VehicleController::s_reverserInterlocked = false;
bool     VehicleController::s_dirSwitchPrev = true;
float    VehicleController::s_slewHeadDuty = 0.0f;
float    VehicleController::s_slewTailDuty = 0.0f;
float    VehicleController::s_slewCabDuty = 0.0f;
float    VehicleController::s_slewStepDuty = 0.0f;
uint32_t VehicleController::s_lastLightSlewMs = 0;
bool     VehicleController::s_ditchRunning = false;

int16_t  VehicleController::aux_hydraulic1 = 0;
bool     VehicleController::bucket_rattle_trigger = false;
bool     VehicleController::dump_bed_toggle = false;

// ── Method implementations ──

const char* VehicleController::engineStateStr(RcEngineSound::EngineState s) {
    switch (s) {
        case RcEngineSound::OFF: return "OFF";
        case RcEngineSound::STARTING: return "STARTING";
        case RcEngineSound::RUNNING: return "RUNNING";
        case RcEngineSound::STOPPING: return "STOPPING";
        case RcEngineSound::PARKING_BRAKE: return "PARKING_BRAKE";
        default: return "UNKNOWN";
    }
}

void VehicleController::applyConfiguredLightMask(const HardwareConfig::Lights& L, bool auxHwConfigured) {
    uint8_t truckMask = HardwareInit::getConfiguredLightMask(L, false);
    uint8_t locoMask  = HardwareInit::getConfiguredLightMask(L, true);

    truck_light.setItemMask(truckMask);
    loco_light.setItemMask(locoMask);

    Serial.printf("[Lights] Configured Mask: Truck=0x%02X, Loco=0x%02X\n", truckMask, locoMask);
}

void VehicleController::init(HardwareConfig* hw, RcEngineSound* engine, VehicleProfile* profile) {
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

    // Initialize locomotive throttle slider at neutral idle (-100 / 0%)
    throttle_slider.rk.value = -100;

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
    s_turnModePrev = HardwareInit::TurnMode::OFF;
    s_lastIndicatorClick = 0;
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

    s_currentSteerAngle = 0.0f;
    s_lastSteerInputVal = 0;
    s_lastSteerTouchMs = 0;
    s_lastSteerPhysicsMs = 0;

    // ── Battery Cell Count: config-driven, voltage auto-detect as fallback ──
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
    s_wasConnected = RadioKit.isConnected();
    s_reconnectThrottleInterlock = false;
    s_disconnectEngineStopDone = false;

    Serial.printf("[VehicleController] Battery warning: %.2fV (%.2fV/cell), cutoff: %.2fV (%.2fV/cell)\n",
                  s_warningVoltage, s_hw->battery.warningVoltage,
                  s_cutoffVoltage, s_hw->battery.cutoffVoltage);

    // ── Reset Locomotive Dynamics & Lighting State ──
    s_currentMotorSpeed = 0.0f;
    s_lastInertiaTime = 0;
    s_prevMotorMoving = false;
    s_locoInitialized = false;
    s_activeDirection = true;
    s_reverserInterlocked = false;
    s_dirSwitchPrev = true;
    s_slewHeadDuty = 0.0f;
    s_slewTailDuty = 0.0f;
    s_slewCabDuty = 0.0f;
    s_slewStepDuty = 0.0f;
    s_lastLightSlewMs = 0;
    s_ditchRunning = false;

    // ── Vehicle type boot visibility & UI page binding ──
    const RcEngineSound::VehicleType t = s_profile->config.type;
    if (t == RcEngineSound::VEHICLE_EXCAVATOR) {
        Serial.println("[VehicleController] Vehicle type: EXCAVATOR (control surface deferred — using truck widget set)");
        RadioKit.setActivePage(0);
    } else if (t == RcEngineSound::VEHICLE_LOCOMOTIVE) {
        Serial.println("[VehicleController] Vehicle type: LOCOMOTIVE");
        RadioKit.setActivePage(1);
    } else if (t == RcEngineSound::VEHICLE_UNKNOWN) {
        Serial.println("[VehicleController] Vehicle type: UNKNOWN (defaulting to truck widget set)");
        RadioKit.setActivePage(0);
    } else {
        Serial.println("[VehicleController] Vehicle type: TRUCK");
        RadioKit.setActivePage(0);
    }
}

void VehicleController::update() {
    if (!s_hw || !s_engine || !s_profile) return;

    // ── Power Button Click Handler ──
    if (HardwareInit::consumeButtonClicked()) {
        s_disconnectStart = millis();
        s_inWarningPhase = false;
    }

    // ── Connection Status & Failsafe Transition Tracking ──
    bool isConnected = RadioKit.isConnected();
    if (isConnected && !s_wasConnected) {
        s_reconnectThrottleInterlock = true;
        HardwareInit::wakeServos();
        s_disconnectEngineStopDone = false;
        Serial.println("[EVENT] Controller reconnected -> throttle interlock armed");
    } else if (!isConnected && s_wasConnected) {
        HardwareInit::sleepServos();
        HardwareInit::setAuxMotor(0);
        HardwareInit::setPump(false);
        s_disconnectEngineStopDone = false;
        Serial.println("[EVENT] Controller disconnected -> failsafe engaged (50% brake, servos asleep)");
    }
    s_wasConnected = isConnected;

    // ── 3-State Board Power & Disconnect Auto Power-Off ──
    bool isCharging = HardwareInit::isCharging();
    if (isCharging && !isConnected) {
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
    } else if (isConnected) {
        s_disconnectStart = 0;
        s_inWarningPhase = false;
        if (!HardwareInit::isPowerLatched()) {
            HardwareInit::latchPower();
        }
    } else {
        // Disconnected state
        if (s_disconnectStart == 0) {
            s_disconnectStart = millis();
        }
        uint32_t elapsed = millis() - s_disconnectStart;

        // Auto-stop engine after 30s of disconnect
        if (elapsed >= 30000 && !s_disconnectEngineStopDone) {
            s_disconnectEngineStopDone = true;
            if (s_engine->getState() != RcEngineSound::OFF) {
                s_engine->stopEngine();
                Serial.println("[EVENT] 30s disconnect reached -> engine auto-stop");
            }
        }

        uint32_t timeoutMs = (uint32_t)s_hw->power.disconnectTimeoutS * 1000U;
        uint32_t warningMs = (uint32_t)s_hw->power.warningWindowS * 1000U;
        if (timeoutMs > 0) {
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

    // Vehicle type from vehicle-config.json is the single source of truth
    const RcEngineSound::VehicleType vtype = s_profile->config.type;
    const bool isLoco = (vtype == RcEngineSound::VEHICLE_LOCOMOTIVE);

    // ── Battery Protection & Low Voltage Cutoff ──
    static uint32_t s_lastBatSample = 0;
    uint32_t nowMs = millis();
    if (nowMs - s_lastBatSample >= 200) {
        s_lastBatSample = nowMs;
        if (POWER::VOLTAGE_SENS != 0xFF) {
            float pinV = analogReadMilliVolts(POWER::VOLTAGE_SENS) / 1000.0f;
            float rawBatV = pinV * s_hw->battery.vScale + s_hw->battery.vOffset;

            if (rawBatV > 2.5f * s_cellCount) {
                if (s_filteredBatV <= 0.1f) {
                    s_filteredBatV = rawBatV;
                } else {
                    s_filteredBatV = 0.90f * s_filteredBatV + 0.10f * rawBatV;
                }
            } else {
                s_filteredBatV = 0.0f;
            }
        }
    }
    float batV = s_filteredBatV;

    if (s_cutoffVoltage > 0.0f && batV > 2.5f * s_cellCount) {
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
    } else {
        s_batteryWarning = false;
        s_batteryCutoff = false;
        s_lowVoltageStart = 0;
    }

    if (s_batteryCutoff) {
        HardwareInit::setAllMotors(0);
        s_engine->triggerOutOfFuel(true);
        if (!s_cutoffLightResetDone) {
            s_cutoffLightResetDone = true;
            UiLogger::logf("CRITICAL: Battery cutoff (%.2fV)", batV);
            HardwareInit::stopLightAnimations();
        }
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
    if (isConnected) {
        int16_t rawPedal = isLoco ? constrain((int16_t)throttle_slider.rk.value, (int16_t)-100, (int16_t)100)
                                  : constrain(gas_pedal.rk.value, (int16_t)-100, (int16_t)100);
        int16_t calculatedPct = (rawPedal + 100) / 2;

        if (s_reconnectThrottleInterlock) {
            if (rawPedal <= -90 || calculatedPct == 0) {
                s_reconnectThrottleInterlock = false;
                throttlePct = calculatedPct;
                Serial.println("[EVENT] Reconnect throttle zero-crossing confirmed -> drive unlocked");
            } else {
                throttlePct = 0;
            }
        } else {
            throttlePct = calculatedPct;
        }
    } else {
        throttlePct = 0;
    }

    // ── Engine Start / Power State Machine ──
    RcEngineSound::EngineState eState = s_engine->getState();
    if (eState != s_engineStatePrev) {
        Serial.printf("[EVENT] EngineState -> %s (RPM: %u)\n", engineStateStr(eState), s_engine->getRpm());
        s_engineStatePrev = eState;
    }
    uint8_t bits = isLoco ? loco_light.rk.value : truck_light.rk.value;
    bool engineStartToggle = isLoco ? engine_button.rk.state : start_button.rk.state;

    if (isConnected) {
        // Auto Interlock on Engine Start / Stop
        if (!isLoco) {
            if (engineStartToggle && !s_engineStartTogglePrev) {
                gear_switch.rk.value = 0;
            } else if (!engineStartToggle && s_engineStartTogglePrev) {
                gear_switch.rk.value = 1;
            } else if (!engineStartToggle && gear_switch.rk.value != 1) {
                gear_switch.rk.value = 1;
            }
        } else {
            if (engineStartToggle && !s_engineStartTogglePrev) {
                throttle_slider.rk.value = -100;
            }
        }

        if (eState == RcEngineSound::OFF) {
            if (engineStartToggle) {
                s_engine->startEngine();
            }
            HardwareInit::setAllMotors(0);
            throttlePct = 0;
        } else if (eState == RcEngineSound::STARTING) {
            if (!engineStartToggle && s_engineStartTogglePrev) s_engine->stopEngine();
            HardwareInit::setAllMotors(0);
            throttlePct = 0;
        } else if (eState == RcEngineSound::RUNNING) {
            if (!engineStartToggle) {
                if (s_engineStartTogglePrev) s_engine->stopEngine();
                throttlePct = 0;
            }
        }

        s_engineStartTogglePrev = engineStartToggle;
    } else {
        // When disconnected, throttle is 0 and engine stays at idle until 30s auto-stop
        if (eState != RcEngineSound::RUNNING) {
            HardwareInit::setAllMotors(0);
            throttlePct = 0;
        }
    }

    // ── Direction / Gear (type-driven) ──
    bool reverse;
    bool parkingBrake = false;
    uint8_t gear = 1;
    int16_t brakePct = 0;

    if (isLoco) {
        if (!s_locoInitialized) {
            s_activeDirection = dir_switch.rk.state;
            s_dirSwitchPrev = dir_switch.rk.state;
            s_reverserInterlocked = false;
            s_locoInitialized = true;
        }

        bool userDir = dir_switch.rk.state;
        if (userDir != s_dirSwitchPrev) {
            s_dirSwitchPrev = userDir;
            if (fabs(s_currentMotorSpeed) > 0.5f) {
                s_reverserInterlocked = true;
                Serial.printf("[EVENT] Reverser flipped to %s while moving (Speed: %.1f) -> Dynamic braking interlock\n",
                              userDir ? "FWD" : "REV", s_currentMotorSpeed);
            } else {
                s_activeDirection = userDir;
                s_reverserInterlocked = false;
                Serial.printf("[EVENT] Reverser switched to %s (Stationary)\n", userDir ? "FWD" : "REV");
            }
        }

        if (s_reverserInterlocked) {
            throttlePct = 0;
            throttle_slider.rk.value = -100;
            brakePct = 100;
            if (fabs(s_currentMotorSpeed) < 0.5f) {
                s_currentMotorSpeed = 0.0f;
                s_activeDirection = userDir;
                s_reverserInterlocked = false;
                Serial.printf("[EVENT] Reverser interlock cleared -> Engaged %s\n",
                              s_activeDirection ? "FWD" : "REV");
            }
        }

        reverse = !s_activeDirection;
    } else {
        if (isConnected) {
            uint8_t g = gear_switch.rk.value;
            if (g <= 2) gear = g;
            reverse = (gear == 2);
            parkingBrake = (gear == 1);
            if (gear != s_gearPrev) {
                if (eState == RcEngineSound::RUNNING) s_engine->triggerShifting(true);
                Serial.printf("[EVENT] Gear -> %s\n", gear == 0 ? "D" : gear == 1 ? "P" : "R");
                s_gearPrev = gear;
            }
        } else {
            // Disconnected failsafe: maintain motion until stopped, then lock in Park
            reverse = (s_gearPrev == 2);
            if (fabs(s_currentMotorSpeed) < 0.1f) {
                gear = 1;
                parkingBrake = true;
                if (s_gearPrev != 1) {
                    s_gearPrev = 1;
                    Serial.println("[EVENT] Failsafe stop complete -> Engaged Park (P)");
                }
            } else {
                gear = s_gearPrev;
                parkingBrake = false;
            }
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

    if (isConnected) {
        if (!isLoco) {
            int16_t rawBrake = constrain(brake_pedal.rk.value, -100, 100);
            brakePct = (rawBrake + 100) / 2;
        }
    } else {
        // 50% braking stop on disconnect
        brakePct = 50;
    }
    int16_t motorThrottle = throttlePct;
    if (parkingBrake) {
        motorThrottle = 0;
    } else if (brakePct > 20) {
        motorThrottle = (int16_t)((int32_t)throttlePct * (100 - brakePct) / 80);
    }
    int16_t targetSpeed = reverse ? -motorThrottle : motorThrottle;

    int16_t motorSpeed = computeRampedMotorSpeed(targetSpeed, parkingBrake, gear, brakePct, eState);

    // ── Steering (with Dynamic Auto-Centering) ──
    int16_t rawSteer = (isLoco || !isConnected) ? 0 : steering_wheel.rk.value;
    int16_t steerVal = updateDynamicSteering(rawSteer, motorSpeed, reverse, isLoco);

    if (s_hw->drivetrainType == HardwareConfig::SKID_STEER) {
        if (eState == RcEngineSound::RUNNING) {
            int16_t sens = s_hw->steeringSensitivity;
            int16_t linearSpeed = abs(motorSpeed);
            int16_t leftSpeed = linearSpeed + (steerVal * sens / 100);
            int16_t rightSpeed = linearSpeed - (steerVal * sens / 100);
            leftSpeed = constrain(leftSpeed, -100, 100);
            rightSpeed = constrain(rightSpeed, -100, 100);
            if (reverse) { leftSpeed = -leftSpeed; rightSpeed = -rightSpeed; }
            if (parkingBrake) { leftSpeed = 0; rightSpeed = 0; }
            HardwareInit::setSkidMotors(leftSpeed, rightSpeed);
        } else {
            HardwareInit::setSkidMotors(0, 0);
        }
    } else {
        if (eState == RcEngineSound::RUNNING && !parkingBrake) {
            HardwareInit::setMotor(motorSpeed);
        } else {
            HardwareInit::setMotor(0);
        }
        if (isConnected) {
            HardwareInit::setServo(steerVal);
        }
    }

    // ── Work Machine Aux Outputs ──
    aux_hydraulic1 = (isLoco || !isConnected) ? 0 : aux_slider.rk.value;
    HardwareInit::setAuxMotor(aux_hydraulic1);
    if (!isConnected) {
        HardwareInit::setPump(false);
    }

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
        bucket_rattle_trigger = false;
    }

    if (s_profile->config.features.hydraulicEnabled) {
        s_engine->triggerHydraulicPump(eState == RcEngineSound::RUNNING);
    }

    if (s_profile->config.features.trackRattleEnabled) {
        bool vehicleMoving = (throttlePct > 0 && eState == RcEngineSound::RUNNING);
        s_engine->triggerTrackRattle(vehicleMoving);
    }

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

    // ── Real Vehicle Turn Indicator Auto-Cancellation ──
    bool hazardActive = isLoco ? ((bits & 0x80) != 0 || !isConnected) : ((bits & 0x08) || s_inWarningPhase || !isConnected);

    if (!isLoco) {
        if (hazardActive) {
            // When hazard is active, turn indicators are immediately switched OFF, disarmed, and ignored
            if (s_leftIndActive || s_rightIndActive) {
                Serial.println("[EVENT] Turn indicator overridden by Hazard switch");
            }
            s_leftIndActive = false;
            s_rightIndActive = false;
            s_leftTurnArmed = false;
            s_rightTurnArmed = false;
            s_leftIndSuppressed = true;
            s_rightIndSuppressed = true;
            left_indicator.rk.state = false;
            right_indicator.rk.state = false;
            s_leftIndPrev = false;
            s_rightIndPrev = false;
        } else {
            bool rawLeft = left_indicator.rk.state;
            bool rawRight = right_indicator.rk.state;

            if (!rawLeft) s_leftIndSuppressed = false;
            if (!rawRight) s_rightIndSuppressed = false;

            bool effLeft = rawLeft && !s_leftIndSuppressed;
            bool effRight = rawRight && !s_rightIndSuppressed;

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
                    s_leftIndActive = false;
                    s_leftIndSuppressed = true;
                    s_leftTurnArmed = false;
                    Serial.println("[EVENT] Left indicator auto-cancelled (opposite steering)");
                } else if (delta <= -20) {
                    s_leftTurnArmed = true;
                    if (steerVal < s_leftTurnPeak) s_leftTurnPeak = steerVal;
                } else if (s_leftTurnArmed && (steerVal >= s_leftTurnPeak + 15 || steerVal >= -5)) {
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
                    s_rightIndActive = false;
                    s_rightIndSuppressed = true;
                    s_rightTurnArmed = false;
                    Serial.println("[EVENT] Right indicator auto-cancelled (opposite steering)");
                } else if (delta >= 20) {
                    s_rightTurnArmed = true;
                    if (steerVal > s_rightTurnPeak) s_rightTurnPeak = steerVal;
                } else if (s_rightTurnArmed && (steerVal <= s_rightTurnPeak - 15 || steerVal <= 5)) {
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
    uint8_t headlightMode = 0;
    bool fogLamp = false;
    if (!isLoco) {
        bool headLight = (bits & 0x01);
        bool highBeam  = (bits & 0x02);
        fogLamp        = (bits & 0x04);

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
        s_headlightMode = (bits & 0x01) ? 1 : 0;
        headlightMode = s_headlightMode;
    }

    // ── Apply Lights with Automation ──
    applyLightsWithAutomation(bits,
                             turnSignalL,
                             turnSignalR,
                             decelBrakeActive,
                             brakePressed,
                             headlightMode,
                             !isLoco && reverse,
                             fogLamp,
                             isLoco);

    // ── Synchronized Indicator Audio Click Sound ──
    uint32_t now = millis();
    HardwareInit::TurnMode curTurnMode = HardwareInit::getTurnMode();
    if (curTurnMode != HardwareInit::TurnMode::OFF) {
        uint16_t onMs = (s_hw && s_hw->lights.turnLight.intervalOn) ? s_hw->lights.turnLight.intervalOn : 500;
        uint16_t offMs = (s_hw && s_hw->lights.turnLight.intervalOff) ? s_hw->lights.turnLight.intervalOff : 500;
        uint32_t period = onMs + offMs;
        if (period == 0) period = 1000;

        if (curTurnMode != s_turnModePrev || (now - s_lastIndicatorClick >= period)) {
            s_engine->triggerIndicator(true);
            s_lastIndicatorClick = now;
        }
    } else if (s_turnModePrev != HardwareInit::TurnMode::OFF) {
        s_engine->triggerIndicator(false);
    }
    s_turnModePrev = curTurnMode;

    // ── Telemetry & Serial Debug Stream ──
    if (now - s_lastTelemetry >= 1000) {
        s_lastTelemetry = now;
        updateTelemetry(motorSpeed, steerVal, throttlePct, gear, brakePressed, turnSignalL, turnSignalR, bits, batV);
    }
}

// ── Private method implementations ──

int16_t VehicleController::computeRampedMotorSpeed(int16_t targetSpeed, bool parkingBrake, uint8_t gear, int16_t brakePct, RcEngineSound::EngineState eState) {
    bool directMode = (!s_profile || !s_profile->config.engine.hasEngine || s_profile->config.engine.inertia == 0);
    if (directMode) {
        s_currentMotorSpeed = (float)targetSpeed;
        return targetSpeed;
    }

    if (parkingBrake || eState == RcEngineSound::OFF) {
        s_currentMotorSpeed = 0.0f;
        s_lastInertiaTime = millis();
        return 0;
    }

    uint32_t now = millis();
    uint32_t dt = (s_lastInertiaTime == 0) ? 20 : (now - s_lastInertiaTime);
    if (dt == 0) return (int16_t)roundf(s_currentMotorSpeed);
    s_lastInertiaTime = now;

    if (dt > 100) dt = 100;

    uint16_t rampInterval = (s_profile->config.engine.escRampTime > 0) ? s_profile->config.engine.escRampTime : 20;
    if (s_profile->config.transmission.type != RcEngineSound::TRANS_NONE && gear < 6) {
        uint8_t gRamp = s_profile->config.transmission.gearRampTimes[gear];
        if (gRamp > 0) rampInterval = gRamp;
    }

    float timeFactor = (float)dt / (float)rampInterval;
    float diff = (float)targetSpeed - s_currentMotorSpeed;

    if (fabs(diff) > 0.01f) {
        float step = 1.0f;
        bool isAccelerating = (diff > 0 && s_currentMotorSpeed >= 0) || (diff < 0 && s_currentMotorSpeed <= 0);

        if (isAccelerating) {
            float accStep = (float)s_profile->config.engine.acc;
            if (accStep < 1.0f) accStep = 2.0f;
            step = max(0.5f, accStep * timeFactor);
        } else if (brakePct > 20 || (targetSpeed == 0 && brakePct > 0)) {
            float brakeDec = (float)s_profile->config.engine.brakeDec;
            if (brakeDec < 1.0f) brakeDec = 10.0f;
            float brakeScale = (float)brakePct / 100.0f;
            step = max(1.0f, brakeDec * brakeScale * timeFactor);
        } else {
            float decStep = (float)s_profile->config.engine.dec;
            if (decStep < 0.5f) decStep = 2.0f;
            step = max(0.5f, decStep * timeFactor);
        }

        if (diff > 0) {
            s_currentMotorSpeed = min((float)targetSpeed, s_currentMotorSpeed + step);
        } else {
            s_currentMotorSpeed = max((float)targetSpeed, s_currentMotorSpeed - step);
        }
    }

    if (s_prevMotorMoving && fabs(s_currentMotorSpeed) < 1.0f && targetSpeed == 0) {
        if (s_engine) s_engine->triggerBrake(true);
        s_prevMotorMoving = false;
    } else if (fabs(s_currentMotorSpeed) > 10.0f) {
        s_prevMotorMoving = true;
    }

    return (int16_t)roundf(s_currentMotorSpeed);
}

int16_t VehicleController::updateDynamicSteering(int16_t rawSteer, int16_t motorSpeed, bool reverse, bool isLoco) {
    if (isLoco) {
        s_currentSteerAngle = 0.0f;
        s_lastSteerInputVal = 0;
        return 0;
    }

    uint32_t now = millis();
    uint32_t dt = (s_lastSteerPhysicsMs == 0) ? 20 : (now - s_lastSteerPhysicsMs);
    if (dt > 100) dt = 100;
    s_lastSteerPhysicsMs = now;

    int8_t currentInputVal = (int8_t)constrain(rawSteer, -100, 100);

    // Track active user interaction: value changed from app or widget active flag
    if (currentInputVal != s_lastSteerInputVal) {
        s_lastSteerTouchMs = now;
        s_lastSteerInputVal = currentInputVal;
        s_currentSteerAngle = (float)currentInputVal;
    }

    bool isInteracting = (steering_wheel.rk.active) || (s_lastSteerTouchMs > 0 && (now - s_lastSteerTouchMs < 120));

    if (!s_hw || !s_hw->autoCentering.enabled || isInteracting) {
        s_currentSteerAngle = (float)currentInputVal;
        return currentInputVal;
    }

    // Dynamic auto-centering calculation
    const auto& ac = s_hw->autoCentering;
    float absSpeed = fabs((float)motorSpeed);

    float rate = ac.baseRate;
    if (!(reverse && ac.holdInReverse)) {
        rate += ac.speedRate * (absSpeed / 100.0f);
    }
    if (rate > ac.maxRate) rate = ac.maxRate;

    float timeFactor = (float)dt / 20.0f; // normalized to 50Hz (20ms) loop
    float step = rate * timeFactor;

    if (s_currentSteerAngle > 0.0f) {
        s_currentSteerAngle = max(0.0f, s_currentSteerAngle - step);
    } else if (s_currentSteerAngle < 0.0f) {
        s_currentSteerAngle = min(0.0f, s_currentSteerAngle + step);
    }

    int8_t decayedSteer = (int8_t)roundf(s_currentSteerAngle);

    // Synchronize decayed position back to RadioKit widget to unwind UI wheel
    if (steering_wheel.rk.value != decayedSteer) {
        steering_wheel.rk.value = decayedSteer;
        s_lastSteerInputVal = decayedSteer;
    }

    return (int16_t)decayedSteer;
}

void VehicleController::applyLightsWithAutomation(uint8_t bits, bool turnL, bool turnR, bool decelBrake, bool manualBrake, uint8_t headlightMode, bool autoReverseLight, bool fogLamp, bool isLoco) {
    if (!s_hw) return;
    const HardwareConfig::Lights& L = s_hw->lights;

    if (isLoco) {
        uint8_t targetHead = 0;
        uint8_t targetTail = 0;
        if (headlightMode > 0) {
            if (s_activeDirection) {
                targetHead = L.headLight.brightness;
                targetTail = 0;
            } else {
                targetHead = 0;
                targetTail = L.tailLight.brightness;
            }
        }

        uint8_t targetCab  = (bits & 0x04) ? (L.cabLight.configured ? L.cabLight.brightness : 0) : 0;
        uint8_t targetStep = (bits & 0x08) ? (L.stepLight.configured ? L.stepLight.brightness : 0) : 0;

        uint32_t nowLight = millis();
        uint32_t dt = (s_lastLightSlewMs == 0) ? 20 : (nowLight - s_lastLightSlewMs);
        if (dt > 100) dt = 100;
        s_lastLightSlewMs = nowLight;

        auto slewChannel = [&](float& current, uint8_t target) -> uint8_t {
            float t = (float)target;
            if (t > current) {
                current = min(t, current + 0.5f * (float)dt);
            } else if (t < current) {
                current = max(t, current - 1.0f * (float)dt);
            }
            return (uint8_t)roundf(current);
        };

        uint8_t headDuty = slewChannel(s_slewHeadDuty, targetHead);
        uint8_t tailDuty = slewChannel(s_slewTailDuty, targetTail);
        uint8_t cabDuty  = slewChannel(s_slewCabDuty,  targetCab);
        uint8_t stepDuty = slewChannel(s_slewStepDuty, targetStep);

        HardwareInit::setLight(L.headLight.pin, headDuty);
        HardwareInit::setLight(L.tailLight.pin, tailDuty);
        if (L.cabLight.configured)  HardwareInit::setLight(L.cabLight.pin, cabDuty);
        if (L.stepLight.configured) HardwareInit::setLight(L.stepLight.pin, stepDuty);

        bool ditchActive = (bits & 0x02);
        if (L.ditchLight.configured) {
            if (ditchActive) {
                uint16_t interval = (L.ditchLight.intervalMs <= 20) ? (L.ditchLight.intervalMs * 100) : L.ditchLight.intervalMs;
                if (interval == 0) interval = 600;
                uint32_t cycleMs = 2 * (uint32_t)interval;
                uint32_t t = nowLight % cycleMs;
                float phase = (t < interval) ? ((float)t / (float)interval) : ((float)(cycleMs - t) / (float)interval);
                uint8_t maxDuty = L.ditchLight.brightness;
                uint8_t leftDuty = (uint8_t)roundf(phase * maxDuty);
                uint8_t rightDuty = (uint8_t)roundf((1.0f - phase) * maxDuty);
                HardwareInit::setLight(L.ditchLight.leftPin, leftDuty);
                HardwareInit::setLight(L.ditchLight.rightPin, rightDuty);
                s_ditchRunning = true;
            } else if (s_ditchRunning) {
                HardwareInit::setLight(L.ditchLight.leftPin, 0);
                HardwareInit::setLight(L.ditchLight.rightPin, 0);
                s_ditchRunning = false;
            }
        }

        bool beaconOn = (bits & 0x10);
        HardwareInit::setBeacon(beaconOn);

        uint8_t auxDuty = (bits & 0x20) ? L.auxLight.brightness : 0;
        if (L.auxLight.configured) {
            HardwareInit::setLight(L.auxLight.pin, auxDuty);
        } else if (s_hw->auxLight.configured) {
            HardwareInit::setAuxLight((bits & 0x20) ? s_hw->auxLight.brightness : 0);
        }

        return;
    }

    bool manualTail = false;
    bool brakeActive = manualBrake || decelBrake;
    bool manualRev  = autoReverseLight;

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

    if (headBright != s_lastHeadBright) {
        s_lastHeadBright = headBright;
        HardwareInit::setLightFade(L.headLight.pin, headBright, s_hw->animation.fadeDurationMs);
    }
    if (L.fullBeam.configured && fullBright != s_lastFullBright) {
        s_lastFullBright = fullBright;
        HardwareInit::setLightFade(L.fullBeam.pin, fullBright, s_hw->animation.fadeDurationMs);
    }

    uint8_t headLive = HardwareInit::getLightDutyPercent(L.headLight.pin);
    uint8_t tailBright = manualTail ? L.tailLight.brightness
                                   : (uint8_t)(headLive * 0.30f);

    uint8_t brakeBright = brakeActive ? L.brakeLight.brightness : 0;
    uint8_t revBright   = manualRev ? L.reversingLight.brightness : 0;

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

    uint8_t fogDuty = (bits & 0x04) ? L.fogLamp.brightness : 0;
    if (L.fogLamp.configured) {
        HardwareInit::setLight(L.fogLamp.pin, fogDuty);
    }

    bool beaconOn = (bits & 0x10);
    HardwareInit::setBeacon(beaconOn);

    uint8_t cabDuty = (bits & 0x20) ? L.cabLight.brightness : 0;
    if (L.cabLight.configured) {
        HardwareInit::setLight(L.cabLight.pin, cabDuty);
    }

    uint8_t workDuty = (bits & 0x40) ? L.workLight.brightness : 0;
    if (L.workLight.configured) {
        HardwareInit::setLight(L.workLight.pin, workDuty);
    }

    uint8_t auxDuty = (bits & 0x80) ? L.auxLight.brightness : 0;
    if (L.auxLight.configured) {
        HardwareInit::setLight(L.auxLight.pin, auxDuty);
    } else if (s_hw->auxLight.configured) {
        HardwareInit::setAuxLight((bits & 0x80) ? s_hw->auxLight.brightness : 0);
    }

    bool hazardActive = (bits & 0x08) || s_inWarningPhase || !RadioKit.isConnected();
    HardwareInit::setTurnSignals(turnL, turnR, hazardActive, L.turnLight.intervalOn, L.turnLight.intervalOff, L.turnLight.brightness);
}

void VehicleController::updateTelemetry(int16_t motorSpeed, int16_t steerVal, int16_t throttlePct, uint8_t gear, bool brakePressed, bool turnL, bool turnR, uint8_t bits, float batV) {
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

    int speedKmph = abs(motorSpeed) * 2;
    snprintf(s_speedBuf, sizeof(s_speedBuf), "%d", speedKmph);
    telemetry_Speed.rk.content = s_speedBuf;

    const char* gearStr = (gear == 0) ? "D" : (gear == 2) ? "R" : "P";
    const char* eStateStr = s_engine ? engineStateStr(s_engine->getState()) : "OFF";
    uint16_t rpm = s_engine ? s_engine->getRpm() : 0;
    Serial.printf("[STATUS] Eng:%s RPM:%u Thr:%d%% Mot:%d%% Spd:%dkm/h Steer:%d Gear:%s Brk:%d Head:%d L:%d R:%d Bat:%.2fV (%s%%)\n",
                  eStateStr, rpm, throttlePct, motorSpeed, speedKmph, steerVal, gearStr,
                  brakePressed ? 1 : 0, s_headlightMode, turnL ? 1 : 0, turnR ? 1 : 0,
                  batV, s_battBuf);
}
