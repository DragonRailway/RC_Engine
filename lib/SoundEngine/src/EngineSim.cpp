#include "EngineSim.h"
#include <cmath>

EngineSim::EngineSim() :
    state(OFF),
    currentRpm(0.0f),
    currentRpmFixed(0),
    pitchFactor(1.0f),
    currentThrottleFaded(0.0f),
    currentMotorSpeed(0.0f),
    lastInertiaTime(0),
    prevMotorMoving(false),
    invMaxRpm(1.0f / 500.0f),
    pitchRange(0.4f),
    gearSize(0),
    selectedGear(1),
    lastGear(1),
    virtualSpeed(0),
    brakeActive(false),
    reverseActive(false),
    jakeBrakeActive(false),
    jakeBrakeRequest(false),
    wastegateTriggered(false),
    gearShiftTriggered(false),
    brakeSquealTriggered(false),
    tireSquealActive(false),
    tireSquealVolume(0),
    trackRattleTriggered(false),
    hydraulicPumpActive(false),
    hydraulicPumpVolume(0),
    lastThrottle(0),
    lastUpdateTime(0),
    stopStartMillis(0),
    stopDurationMs(1400),
    stopPitchFactor(1.0f),
    lastTrackRattleTime(0),
    wastegateTriggerMillis(0),
    crawlerMode(false),
    engineStopRequested(false)
{
}

void EngineSim::begin(const Config& config) {
    setConfig(config);
    currentRpm = 0.0f;
    currentRpmFixed = 0;
    currentMotorSpeed = 0.0f;
    virtualSpeed = 0;
    selectedGear = 1;
    lastGear = 1;
    state = OFF;
    engineStopRequested = false;
    lastInertiaTime = 0;
    lastUpdateTime = 0;
}

void EngineSim::setConfig(const Config& config) {
    cfg = config;
    invMaxRpm = (cfg.engine.maxRpm > 0) ? (1.0f / (float)cfg.engine.maxRpm) : (1.0f / 500.0f);
    pitchRange = cfg.engine.maxPitchFactor - 1.0f;
    if (cfg.transmission.numberOfGears > 0) {
        gearSize = cfg.engine.maxRpm / cfg.transmission.numberOfGears;
    } else {
        gearSize = 0;
    }
}

void EngineSim::startEngine() {
    if (state == OFF) {
        state = STARTING;
        virtualSpeed = 0;
        currentMotorSpeed = 0.0f;
        currentRpm = 0.0f;
        currentRpmFixed = 0;
        engineStopRequested = false;
        startStartMillis = millis();
    }
}

void EngineSim::stopEngine() {
    if (state == RUNNING) {
        engineStopRequested = true;
    }
}

void EngineSim::update(int16_t throttle) {
    Input input;
    int16_t throttlePct = (cfg.engine.maxRpm > 0) ? (int16_t)((int32_t)abs(throttle) * 100 / cfg.engine.maxRpm) : (int16_t)abs(throttle);
    input.throttle = (throttle < 0) ? -throttlePct : throttlePct;
    input.brake = 0;
    input.gear = (throttle < 0) ? 2 : 0;
    input.parkingBrake = false;
    input.auxLoad = 0;
    update(input, 20);
}

void EngineSim::update(const Input& input, uint32_t dtMs) {
    uint32_t now = millis();
    if (dtMs == 0) {
        dtMs = (lastUpdateTime > 0 && now >= lastUpdateTime) ? (now - lastUpdateTime) : 20;
    }
    if (dtMs > 100) dtMs = 100;
    if (dtMs < 1) dtMs = 1;
    lastUpdateTime = now;

    if (state == STARTING) {
        if (now >= startStartMillis && (now - startStartMillis > 2000)) {
            state = RUNNING;
        }
    }

    float dtSec = (float)dtMs / 1000.0f;
    float timeFactor = dtSec / 0.02f; // Normalized to 20ms base tick

    // ── Direction / Gear / Parking brake ──
    reverseActive = (input.gear == 2);
    bool parkingBrake = (input.gear == 1 || input.parkingBrake);
    brakeActive = (input.brake > 20);

    // ── Throttle & Auxiliary Hydraulic Governor ──
    int32_t throttlePct = constrain((int32_t)abs(input.throttle), 0, 100);
    int32_t targetRpm = throttlePct * (int32_t)cfg.engine.maxRpm / 100;
    if (input.auxLoad > 0 && state == RUNNING) {
        int32_t minAuxRpm = (int32_t)cfg.engine.maxRpm * 20 / 100;
        if (targetRpm < minAuxRpm) {
            targetRpm = minAuxRpm;
            throttlePct = 20;
        }
    }
    if (targetRpm > cfg.engine.maxRpm) targetRpm = cfg.engine.maxRpm;

    // ── Throttle Slew-Rate Inertia (Fading) ──
    int32_t targetThrottlePct = throttlePct;
    if ((float)targetThrottlePct > currentThrottleFaded) {
        currentThrottleFaded = (float)targetThrottlePct;
    } else {
        float decelRate = max(1.0f, (float)cfg.engine.dec * 0.8f) * timeFactor;
        currentThrottleFaded -= decelRate;
        if (currentThrottleFaded < (float)targetThrottlePct) {
            currentThrottleFaded = (float)targetThrottlePct;
        }
    }

    // ── Crawler Mode Detection ──
    crawlerMode = (cfg.sound.master <= cfg.sound.crawlerModeThreshold);

    // ── Engine Load Calculation ──
    int32_t engineLoad = targetRpm - (int32_t)currentRpm;
    if (engineLoad < 0) engineLoad = 0;
    if (engineLoad > 180) engineLoad = 180;

    // ── Automatic Transmission Simulation ──
    int32_t effectiveTarget = targetRpm;
    if (cfg.transmission.type == TRANS_AUTOMATIC && state == RUNNING) {
        if (gearSize > 0) {
            int32_t speedRef = (int32_t)fabs(currentMotorSpeed * cfg.engine.maxRpm / 100.0f);
            if (targetRpm > virtualSpeed) {
                virtualSpeed += max((int32_t)4, (int32_t)(cfg.engine.acc * 4));
                if (virtualSpeed > cfg.engine.maxRpm) virtualSpeed = cfg.engine.maxRpm;
            } else if (targetRpm < virtualSpeed) {
                virtualSpeed -= max((int32_t)4, (int32_t)(cfg.engine.dec * 4));
                if (virtualSpeed < 0) virtualSpeed = 0;
            }

            selectedGear = (uint8_t)(speedRef / gearSize);
            if (selectedGear >= cfg.transmission.numberOfGears) selectedGear = cfg.transmission.numberOfGears - 1;
            int32_t gearBase = selectedGear * gearSize;
            int32_t throttleInGear = targetRpm - gearBase;
            if (throttleInGear < 0) throttleInGear = 0;
            if (throttleInGear > gearSize) throttleInGear = gearSize;

            int32_t converterSlip = 0;
            if (cfg.transmission.torqueConverterSlip > 0) {
                if (selectedGear == 0) {
                    converterSlip = (engineLoad * (int32_t)cfg.transmission.torqueConverterSlip / 100) * 2;
                } else {
                    converterSlip = (engineLoad * (int32_t)cfg.transmission.torqueConverterSlip / 100);
                }
            }

            int32_t baseGearTarget = (targetRpm < gearBase) ? targetRpm : (gearBase + throttleInGear);
            effectiveTarget = min((int32_t)cfg.engine.maxRpm, baseGearTarget + converterSlip);
        }
    }

    // ── Manual Transmission Shifting Trigger ──
    if (cfg.transmission.type == TRANS_MANUAL && state == RUNNING) {
        if (gearSize > 0) {
            uint8_t newGear = (uint8_t)(targetRpm / gearSize) + 1;
            if (newGear > cfg.transmission.numberOfGears) newGear = cfg.transmission.numberOfGears;
            if (newGear != lastGear && cfg.sound.shifting > 0) {
                gearShiftTriggered = true;
            }
            lastGear = newGear;
            selectedGear = newGear;
        }
    }

    // ── State Machine & RPM Updates ──
    if (state == RUNNING) {
        // Jake brake auto-detection: throttle released at high RPM
        int32_t jakeMargin = cfg.engine.maxRpm / 50;
        bool throttleReleased = ((float)effectiveTarget + (float)jakeMargin < currentRpm);
        bool aboveJakeThreshold = (currentRpmFixed > (uint16_t)(cfg.engine.maxRpm * cfg.engine.jakeBrakeMinRpm / 100));

        jakeBrakeRequest = (throttleReleased && aboveJakeThreshold && cfg.sound.jakeBrake > 0);
        if (jakeBrakeRequest) {
            jakeBrakeActive = true;
        } else {
            jakeBrakeActive = false;
        }

        if (jakeBrakeActive) {
            float decelStep = (float)cfg.engine.jakeBrakeDecelRate * timeFactor;
            currentRpm -= decelStep;
            if (currentRpm < 0.0f) currentRpm = 0.0f;
        } else if (crawlerMode) {
            currentRpm = (float)effectiveTarget;
        } else {
            float inertiaFactor = (float)max((int32_t)1, (int32_t)(101 - cfg.engine.inertia));
            float diff = (float)effectiveTarget - currentRpm;

            float accelStep = (float)cfg.engine.acc;
            float decelStep = (float)cfg.engine.dec;
            if (cfg.transmission.type == TRANS_AUTOMATIC && selectedGear < 6) {
                accelStep = (float)cfg.transmission.gearRampTimes[selectedGear];
                decelStep = accelStep;
            }

            if (diff > 0.0f) {
                float step = max(0.5f, ((diff * inertiaFactor) / 200.0f + accelStep) * timeFactor);
                currentRpm = min((float)effectiveTarget, currentRpm + step);
            } else if (diff < 0.0f) {
                float step = max(0.5f, (((-diff) * inertiaFactor) / 200.0f + decelStep) * timeFactor);
                currentRpm = max((float)effectiveTarget, currentRpm - step);
            }
        }
        currentRpmFixed = (uint16_t)roundf(currentRpm);

        // Tire squeal: high throttle, low speed
        tireSquealActive = (targetRpm > (int32_t)(cfg.engine.maxRpm * cfg.features.tireSquealThreshold / 100) &&
                            virtualSpeed < (int32_t)(cfg.engine.maxRpm * cfg.features.tireSquealMaxSpeed / 100));
        if (tireSquealActive && cfg.sound.tireSqueal > 0) {
            int32_t speedScale = map(virtualSpeed, 0,
                                     cfg.engine.maxRpm * cfg.features.tireSquealMaxSpeed / 100, 100, 0);
            tireSquealVolume = (uint8_t)(cfg.sound.tireSqueal * constrain(speedScale, 0, 100) / 100);
        } else {
            tireSquealVolume = 0;
        }

        // Hydraulic pump volume calculation
        hydraulicPumpActive = cfg.features.hydraulicEnabled;
        if (hydraulicPumpActive && cfg.sound.hydraulicPump > 0) {
            int32_t pumpScale = map(currentRpmFixed, 0, cfg.engine.maxRpm, 30, 100);
            int32_t vol = cfg.sound.hydraulicPump * constrain(pumpScale, 0, 100) / 100;
            if (cfg.features.hydrostaticMode) {
                int32_t speedScale = map(virtualSpeed, 0, cfg.engine.maxRpm, 50, 100);
                vol = vol * constrain(speedScale, 0, 100) / 100;
            }
            hydraulicPumpVolume = (uint8_t)vol;
        } else {
            hydraulicPumpVolume = 0;
        }

        // Track rattle
        if (cfg.features.trackRattleEnabled && virtualSpeed > 0 && cfg.sound.trackRattle > 0) {
            uint32_t interval = map(virtualSpeed, 0, cfg.engine.maxRpm,
                                    cfg.features.trackRattleIntervalMax, cfg.features.trackRattleIntervalMin);
            if (now - lastTrackRattleTime > interval) {
                trackRattleTriggered = true;
                lastTrackRattleTime = now;
            }
        }

        // Wastegate detection
        int16_t throttleDrop = lastThrottle - targetRpm;
        if (throttleDrop > 150 && currentRpmFixed > 200 && !wastegateTriggered) {
            wastegateTriggered = true;
            wastegateTriggerMillis = now;
        }

        // Engine stop request
        if (engineStopRequested && currentRpm < 30) {
            state = STOPPING;
            engineStopRequested = false;
            stopStartMillis = now;
            stopDurationMs = cfg.engine.stopDuration > 0 ? cfg.engine.stopDuration : 1400;
            stopPitchFactor = pitchFactor;
        }
    } else if (state == OFF) {
        currentRpm = 0.0f;
        currentRpmFixed = 0;
        jakeBrakeActive = false;
        tireSquealActive = false;
        hydraulicPumpActive = false;
        trackRattleTriggered = false;
        wastegateTriggered = false;
        currentMotorSpeed = 0.0f;
    }

    if (cfg.transmission.type != TRANS_AUTOMATIC && state == RUNNING) {
        if (targetRpm > virtualSpeed) {
            virtualSpeed += max((int32_t)1, (int32_t)(cfg.engine.acc * 2));
            if (virtualSpeed > cfg.engine.maxRpm) virtualSpeed = cfg.engine.maxRpm;
        } else if (targetRpm < virtualSpeed) {
            virtualSpeed -= max((int32_t)1, (int32_t)(cfg.engine.dec * 2));
            if (virtualSpeed < 0) virtualSpeed = 0;
        }
    }

    lastThrottle = targetRpm;

    // ── Pitch Factor Calculation ──
    if (state == RUNNING) {
        pitchFactor = 1.0f + (currentRpm * invMaxRpm) * pitchRange;
    } else if (state == STOPPING) {
        uint32_t elapsed = (now >= stopStartMillis) ? (now - stopStartMillis) : 0;
        float progress = (stopDurationMs > 0) ? ((float)elapsed / (float)stopDurationMs) : 1.0f;
        if (progress > 1.0f) progress = 1.0f;

        const float minPitch = 0.18f;
        pitchFactor = stopPitchFactor * (1.0f - progress * (1.0f - minPitch / stopPitchFactor));
        currentRpm = (1.0f - progress) * 20.0f;
        currentRpmFixed = (uint16_t)roundf(currentRpm);

        if (progress >= 1.0f) {
            if (parkingBrake) {
                state = PARKING_BRAKE;
            } else {
                state = OFF;
            }
            pitchFactor = 1.0f;
        }
    } else if (state == PARKING_BRAKE) {
        pitchFactor = 1.0f;
        currentRpm = 0.0f;
        currentRpmFixed = 0;
        if (!parkingBrake) {
            state = OFF;
        }
    } else {
        pitchFactor = 1.0f;
    }

    // ── ESC Motor Speed Ramping (Drivetrain Inertia) ──
    int16_t motorThrottle = (int16_t)targetThrottlePct;
    if (parkingBrake) {
        motorThrottle = 0;
    } else if (input.brake > 20) {
        motorThrottle = (int16_t)((int32_t)targetThrottlePct * (100 - input.brake) / 80);
    }
    int16_t targetSpeed = reverseActive ? -motorThrottle : motorThrottle;

    bool directMode = (!cfg.engine.hasEngine || cfg.engine.inertia == 0);
    if (directMode) {
        currentMotorSpeed = (float)targetSpeed;
    } else if (parkingBrake || state == OFF) {
        currentMotorSpeed = 0.0f;
    } else {
        uint16_t rampInterval = (cfg.engine.escRampTime > 0) ? cfg.engine.escRampTime : 20;
        if (cfg.transmission.type != TRANS_NONE && selectedGear < 6) {
            uint8_t gRamp = cfg.transmission.gearRampTimes[selectedGear];
            if (gRamp > 0) rampInterval = gRamp;
        }

        float speedTimeFactor = (float)dtMs / (float)rampInterval;
        float diff = (float)targetSpeed - currentMotorSpeed;

        if (fabs(diff) > 0.01f) {
            float step = 1.0f;
            bool isAccelerating = (diff > 0 && currentMotorSpeed >= 0) || (diff < 0 && currentMotorSpeed <= 0);

            if (isAccelerating) {
                float accStep = (float)cfg.engine.acc;
                if (accStep < 1.0f) accStep = 2.0f;
                step = max(0.5f, accStep * speedTimeFactor);
            } else if (input.brake > 20 || (targetSpeed == 0 && input.brake > 0)) {
                float bDec = (float)cfg.engine.brakeDec;
                if (bDec < 1.0f) bDec = 10.0f;
                float brakeScale = (float)input.brake / 100.0f;
                step = max(1.0f, bDec * brakeScale * speedTimeFactor);
            } else {
                float decStep = (float)cfg.engine.dec;
                if (decStep < 0.5f) decStep = 2.0f;
                step = max(0.5f, decStep * speedTimeFactor);
            }

            if (diff > 0) {
                currentMotorSpeed = min((float)targetSpeed, currentMotorSpeed + step);
            } else {
                currentMotorSpeed = max((float)targetSpeed, currentMotorSpeed - step);
            }
        }

        if (prevMotorMoving && fabs(currentMotorSpeed) < 1.0f && targetSpeed == 0) {
            brakeSquealTriggered = true;
            prevMotorMoving = false;
        } else if (fabs(currentMotorSpeed) > 10.0f) {
            prevMotorMoving = true;
        }
    }
}
