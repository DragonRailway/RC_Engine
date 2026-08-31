#include "RcEngineSound.h"

RcEngineSound::RcEngineSound() :
    state(OFF),
    currentRpm(0),
    currentRpmFixed(0),
    pitchFactor(1.0f),
    startPos(0),
    lastKnockTriggerSample(0),
    curKnockCylinder(0),
    jakeBrakeActive(false),
    engineMuted(false),
    wastegateTriggered(false),
    wastegateTriggerMillis(0),
    selectedGear(1),
    lastGear(1),
    engineStopRequested(false),
    lastUpdateTime(0),
    stopStartMillis(0),
    stopDurationMs(1400),
    stopVolume(100),
    stopPitchFactor(1.0f),
    crawlerMode(false),
    lastTrackRattleTime(0),
    virtualSpeed(0),
    lastThrottle(0)
{
    for (int i = 0; i < SOUND_COUNT; i++) {
        voices[i] = VoiceState();
    }
}

RcEngineSound::~RcEngineSound() {
    if (sounds.isDynamic) {
        for (int i = 0; i < SOUND_COUNT; i++) {
            if (sounds.slots[i].samples) free(sounds.slots[i].samples);
        }
    }
}

void RcEngineSound::begin(const SoundData& soundData, const Config& config) {
    sounds.clear();
    sounds = soundData;
    cfg = config;
    currentRpm = 0;
    currentRpmFixed = 0;
    virtualSpeed = 0;
    selectedGear = 0;
    lastGear = 0;
    startPos = 0;
    state = OFF;

    struct VoiceDef { bool pitchShifted; bool loop; bool oneShot; };
    VoiceDef voiceDefs[SOUND_COUNT] = {};
    voiceDefs[IDLE]           = {true,  true,  false};
    voiceDefs[REV]            = {true,  true,  false};
    voiceDefs[START]          = {false, false, false}; // Handled separately in state machine
    voiceDefs[KNOCK]          = {false, false, true };
    voiceDefs[TURBO]          = {true,  true,  false};
    voiceDefs[WASTEGATE]      = {false, false, true };
    voiceDefs[HORN]           = {false, true,  false};
    voiceDefs[JAKE_BRAKE]     = {true,  true,  false};
    voiceDefs[FAN]            = {true,  true,  false};
    voiceDefs[SIREN]          = {false, true,  false};
    voiceDefs[BRAKE]          = {false, false, true };
    voiceDefs[PARKING_BRAKE]  = {false, false, true };
    voiceDefs[SHIFTING]       = {false, false, true };
    voiceDefs[REVERSING]      = {false, true,  false};
    voiceDefs[INDICATOR]      = {false, false, true };
    voiceDefs[COUPLING]       = {false, false, true };
    voiceDefs[SUPERCHARGER]   = {true,  true,  false};
    voiceDefs[UNCOUPLING]     = {false, false, true };
    voiceDefs[SOUND1]         = {false, true,  false};
    voiceDefs[TIRE_SQUEAL]    = {false, true,  false};
    voiceDefs[HYDRAULIC_PUMP] = {true,  true,  false};
    voiceDefs[HYDRAULIC_FLOW] = {false, true,  false};
    voiceDefs[TRACK_RATTLE]   = {false, false, true };
    voiceDefs[BUCKET_RATTLE]  = {false, false, true };
    voiceDefs[BELL]           = {false, true,  false};
    voiceDefs[DOOR]           = {false, false, true };
    voiceDefs[SCANNER]        = {false, true,  false};
    voiceDefs[MUSIC]          = {false, true,  false};
    voiceDefs[WHISTLE]        = {false, true,  false};
    voiceDefs[GUN]            = {false, false, true };
    voiceDefs[OUT_OF_FUEL]    = {false, true,  false};
    voiceDefs[OTHERS]         = {false, true,  false};

    for (int i = 0; i < SOUND_COUNT; i++) {
        voices[i].samples = sounds.slots[i].samples;
        voices[i].count = sounds.slots[i].sampleCount;
        voices[i].pitchShifted = voiceDefs[i].pitchShifted;
        voices[i].loop = voiceDefs[i].loop;
        voices[i].oneShot = voiceDefs[i].oneShot;
    }

    // Loop points
    voices[HORN].loopBegin = cfg.loopPoints.hornBegin;
    voices[HORN].loopEnd = cfg.loopPoints.hornEnd;
    voices[SIREN].loopBegin = cfg.loopPoints.sirenBegin;
    voices[SIREN].loopEnd = cfg.loopPoints.sirenEnd;
    voices[REVERSING].loopBegin = cfg.loopPoints.reversingBegin;
    voices[REVERSING].loopEnd = cfg.loopPoints.reversingEnd;
    voices[SOUND1].loopBegin = cfg.loopPoints.sound1Begin;
    voices[SOUND1].loopEnd = cfg.loopPoints.sound1End;

    // Configure default volumes
    applyVoiceVolumes();

    Serial.printf("[RcEngineSound] Initialized with %d voice slots\n", SOUND_COUNT);
}

void RcEngineSound::applyVoiceVolumes() {
    voices[IDLE].volume = cfg.sound.idle;
    voices[REV].volume = cfg.sound.rev;
    voices[TURBO].volume = cfg.sound.turbo;
    voices[KNOCK].volume = cfg.sound.knock;
    voices[WASTEGATE].volume = cfg.sound.wastegate;
    voices[HORN].volume = cfg.sound.horn;
    voices[SIREN].volume = cfg.sound.siren;
    voices[BRAKE].volume = cfg.sound.brake;
    voices[JAKE_BRAKE].volume = cfg.sound.jakeBrake;
    voices[REVERSING].volume = cfg.sound.reversing;
    voices[PARKING_BRAKE].volume = cfg.sound.parkingBrake;
    voices[SHIFTING].volume = cfg.sound.shifting;
    voices[INDICATOR].volume = cfg.sound.indicator;
    voices[COUPLING].volume = cfg.sound.coupling;
    voices[FAN].volume = cfg.sound.fan;
    voices[SUPERCHARGER].volume = cfg.sound.supercharger;
    voices[UNCOUPLING].volume = cfg.sound.uncoupling;
    voices[SOUND1].volume = cfg.sound.sound1;
    voices[TIRE_SQUEAL].volume = cfg.sound.tireSqueal;
    voices[HYDRAULIC_PUMP].volume = cfg.sound.hydraulicPump;
    voices[HYDRAULIC_FLOW].volume = cfg.sound.hydraulicFlow;
    voices[TRACK_RATTLE].volume = cfg.sound.trackRattle;
    voices[BUCKET_RATTLE].volume = cfg.sound.bucketRattle;
    voices[BELL].volume = cfg.sound.bell;
    voices[DOOR].volume = cfg.sound.door;
    voices[SCANNER].volume = cfg.sound.scanner;
    voices[MUSIC].volume = cfg.sound.music;
    voices[WHISTLE].volume = cfg.sound.whistle;
    voices[GUN].volume = cfg.sound.gun;
    voices[OUT_OF_FUEL].volume = cfg.sound.outOfFuel;
    voices[OTHERS].volume = cfg.sound.others;
}

void RcEngineSound::setConfig(const Config& config) {
    cfg = config;
    applyVoiceVolumes();
}

void RcEngineSound::begin(const SoundData& soundData) {
    begin(soundData, cfg);
}

void RcEngineSound::startEngine() {
    if (state == OFF) {
        state = STARTING;
        startPos = 0;
        virtualSpeed = 0;
    }
}

void RcEngineSound::stopEngine() {
    if (state == RUNNING) {
        engineStopRequested = true;
    }
}

void RcEngineSound::triggerHorn(bool active) {
    voices[HORN].active = active;
    if (active) voices[HORN].position = 0;
}

void RcEngineSound::triggerSiren(bool active) {
    voices[SIREN].active = active;
    if (active) voices[SIREN].position = 0;
}

void RcEngineSound::triggerBrake(bool active) {
    voices[BRAKE].active = active;
    if (active) voices[BRAKE].position = 0;
}

void RcEngineSound::triggerParkingBrake(bool active) {
    voices[PARKING_BRAKE].active = active;
    if (active) voices[PARKING_BRAKE].position = 0;
}

void RcEngineSound::triggerJakeBrake(bool active) {
    voices[JAKE_BRAKE].active = active;
    if (active) voices[JAKE_BRAKE].position = 0;
}

void RcEngineSound::triggerWastegate(bool active) {
    voices[WASTEGATE].active = active;
    if (active) voices[WASTEGATE].position = 0;
}

void RcEngineSound::triggerReversing(bool active) {
    voices[REVERSING].active = active;
    if (active) voices[REVERSING].position = 0;
}

void RcEngineSound::triggerShifting(bool active) {
    voices[SHIFTING].active = active;
    if (active) voices[SHIFTING].position = 0;
}

void RcEngineSound::triggerIndicator(bool active) {
    voices[INDICATOR].active = active;
    if (active) voices[INDICATOR].position = 0;
}

void RcEngineSound::triggerCoupling(bool active) {
    voices[COUPLING].active = active;
    if (active) voices[COUPLING].position = 0;
}

void RcEngineSound::triggerUncoupling(bool active) {
    voices[UNCOUPLING].active = active;
    if (active) voices[UNCOUPLING].position = 0;
}

void RcEngineSound::triggerSound1(bool active) {
    voices[SOUND1].active = active;
    if (active) voices[SOUND1].position = 0;
}

void RcEngineSound::triggerTireSqueal(bool active) {
    voices[TIRE_SQUEAL].active = active;
    if (active) voices[TIRE_SQUEAL].position = 0;
}

void RcEngineSound::triggerHydraulicPump(bool active) {
    voices[HYDRAULIC_PUMP].active = active;
    if (active) voices[HYDRAULIC_PUMP].position = 0;
}

void RcEngineSound::triggerHydraulicFlow(bool active) {
    voices[HYDRAULIC_FLOW].active = active;
    if (active) voices[HYDRAULIC_FLOW].position = 0;
}

void RcEngineSound::triggerTrackRattle(bool active) {
    voices[TRACK_RATTLE].active = active;
    if (active) voices[TRACK_RATTLE].position = 0;
}

void RcEngineSound::triggerBucketRattle(bool active) {
    voices[BUCKET_RATTLE].active = active;
    if (active) voices[BUCKET_RATTLE].position = 0;
}

void RcEngineSound::triggerDumpBed(bool active) {
    if (cfg.features.dumpBedEnabled) {
        voices[HYDRAULIC_PUMP].active = active;
        voices[HYDRAULIC_FLOW].active = active;
        if (active) {
            voices[HYDRAULIC_PUMP].position = 0;
            voices[HYDRAULIC_FLOW].position = 0;
        }
    }
}

void RcEngineSound::triggerBell(bool active) {
    voices[BELL].active = active;
    if (active) voices[BELL].position = 0;
}

void RcEngineSound::triggerDoor(bool active) {
    voices[DOOR].active = active;
    if (active) voices[DOOR].position = 0;
}

void RcEngineSound::triggerScanner(bool active) {
    voices[SCANNER].active = active;
    if (active) voices[SCANNER].position = 0;
}

void RcEngineSound::triggerMusic(bool active) {
    voices[MUSIC].active = active;
    if (active) voices[MUSIC].position = 0;
}

void RcEngineSound::triggerWhistle(bool active) {
    voices[WHISTLE].active = active;
    if (active) voices[WHISTLE].position = 0;
}

void RcEngineSound::triggerGun(bool active) {
    voices[GUN].active = active;
    if (active) voices[GUN].position = 0;
}

void RcEngineSound::triggerOutOfFuel(bool active) {
    voices[OUT_OF_FUEL].active = active;
    if (active) voices[OUT_OF_FUEL].position = 0;
}

void RcEngineSound::triggerOthers(bool active) {
    voices[OTHERS].active = active;
    if (active) voices[OTHERS].position = 0;
}

// ─── Advanced Engine State Machine ───────────────────────────────────────────
void RcEngineSound::update(int16_t throttle) {
    uint32_t now = millis();
    uint32_t dtMs = (lastUpdateTime > 0 && now >= lastUpdateTime) ? (now - lastUpdateTime) : 20;
    if (dtMs > 100) dtMs = 100;
    if (dtMs < 1) dtMs = 1;
    lastUpdateTime = now;
    float dtSec = (float)dtMs / 1000.0f;
    float timeFactor = dtSec / 0.02f; // Normalized to 20ms base tick

    int32_t targetRpm = abs(throttle);
    if (targetRpm > cfg.engine.maxRpm) targetRpm = cfg.engine.maxRpm;

    // ── Throttle ratio (0-100%) for volume scaling ──
    int32_t throttlePercent = map(targetRpm, 0, cfg.engine.maxRpm, 0, 100);

    // ── Crawler Mode Detection ──
    crawlerMode = (cfg.sound.master <= cfg.sound.crawlerModeThreshold);

    // ── Engine Load Calculation (used for torque converter slip & dynamic sounds) ──
    int32_t engineLoad = targetRpm - (int32_t)currentRpm;
    if (engineLoad < 0) engineLoad = 0;
    if (engineLoad > 180) engineLoad = 180;

    // ── Automatic Transmission Simulation with Torque Converter Slip ──
    int32_t effectiveTarget = targetRpm;
    if (cfg.transmission.type == TRANS_AUTOMATIC && state == RUNNING) {
        int32_t gearSize = cfg.engine.maxRpm / cfg.transmission.numberOfGears;
        if (gearSize > 0) {
            if (throttle > virtualSpeed) {
                virtualSpeed += max((int32_t)4, (int32_t)(cfg.engine.acc * 4));
                if (virtualSpeed > cfg.engine.maxRpm) virtualSpeed = cfg.engine.maxRpm;
            } else if (throttle < virtualSpeed) {
                virtualSpeed -= max((int32_t)4, (int32_t)(cfg.engine.dec * 4));
                if (virtualSpeed < 0) virtualSpeed = 0;
            }
            selectedGear = (uint8_t)(virtualSpeed / gearSize);
            if (selectedGear >= cfg.transmission.numberOfGears) selectedGear = cfg.transmission.numberOfGears - 1;
            int32_t gearBase = selectedGear * gearSize;
            int32_t throttleInGear = targetRpm - gearBase;
            if (throttleInGear < 0) throttleInGear = 0;
            if (throttleInGear > gearSize) throttleInGear = gearSize;

            int32_t converterSlip = 0;
            if (cfg.transmission.torqueConverterSlip > 0) {
                if (selectedGear == 0) {
                    // 1st / launch gear: 2x slip multiplier simulating stall speed & hydraulic slip
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
        int32_t gearSize = cfg.engine.maxRpm / cfg.transmission.numberOfGears;
        if (gearSize > 0) {
            uint8_t newGear = (uint8_t)(targetRpm / gearSize) + 1;
            if (newGear > cfg.transmission.numberOfGears) newGear = cfg.transmission.numberOfGears;
            if (newGear != lastGear && cfg.sound.shifting > 0) {
                voices[SHIFTING].active = true;
                voices[SHIFTING].position = 0;
            }
            lastGear = newGear;
            selectedGear = newGear;
        }
    }

    if (state == RUNNING) {
        // ── Jake brake auto-detection ──
        // Activation requires RPM to exceed effectiveTarget by >2% of maxRpm
        // (prevents false triggers during automatic transmission gear transitions).
        // Deactivation uses the simple inverse — no margin — so jake brake cannot
        // latch in the active state when RPM settles back into the dead zone.
        int32_t jakeMargin = cfg.engine.maxRpm / 50;
        bool throttleReleased = ((float)effectiveTarget + (float)jakeMargin < currentRpm);
        bool aboveJakeThreshold = (currentRpmFixed > (uint16_t)(cfg.engine.maxRpm * cfg.engine.jakeBrakeMinRpm / 100));

        if (throttleReleased && aboveJakeThreshold && cfg.sound.jakeBrake > 0) {
            jakeBrakeActive = true;
            engineMuted = true;
        } else if (!throttleReleased || currentRpmFixed <= (uint16_t)(cfg.engine.maxRpm * cfg.engine.jakeBrakeMinRpm / 100)) {
            jakeBrakeActive = false;
            engineMuted = false;
        }

        // ── Jake brake RPM deceleration ──
        if (jakeBrakeActive) {
            float decelStep = (float)cfg.engine.jakeBrakeDecelRate * timeFactor;
            currentRpm -= decelStep;
            if (currentRpm < 0.0f) currentRpm = 0.0f;
        } else if (crawlerMode) {
            currentRpm = (float)effectiveTarget;
        } else {
            // ── Normal RPM calculation with continuous float smoothing ──
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

        // ── Tire squeal: high throttle, low speed ──
        bool squealCondition = (throttle > (int32_t)(cfg.engine.maxRpm * cfg.features.tireSquealThreshold / 100) &&
                                virtualSpeed < (int32_t)(cfg.engine.maxRpm * cfg.features.tireSquealMaxSpeed / 100));
        voices[TIRE_SQUEAL].active = squealCondition;
        if (squealCondition && cfg.sound.tireSqueal > 0) {
            int32_t speedScale = map(virtualSpeed, 0,
                                     cfg.engine.maxRpm * cfg.features.tireSquealMaxSpeed / 100, 100, 0);
            voices[TIRE_SQUEAL].volume =
                (uint8_t)(cfg.sound.tireSqueal * constrain(speedScale, 0, 100) / 100);
        }

        // ── Hydraulic pump: RPM-dependent, loops while enabled ──
        voices[HYDRAULIC_PUMP].active = cfg.features.hydraulicEnabled;
        if (cfg.features.hydraulicEnabled && cfg.sound.hydraulicPump > 0) {
            int32_t pumpScale = map(currentRpmFixed, 0, cfg.engine.maxRpm, 30, 100);
            int32_t vol = cfg.sound.hydraulicPump * constrain(pumpScale, 0, 100) / 100;
            if (cfg.features.hydrostaticMode) {
                int32_t speedScale = map(virtualSpeed, 0, cfg.engine.maxRpm, 50, 100);
                vol = vol * constrain(speedScale, 0, 100) / 100;
            }
            voices[HYDRAULIC_PUMP].volume = (uint8_t)vol;
        }

        // ── Track rattle: speed-dependent interval ──
        if (cfg.features.trackRattleEnabled && virtualSpeed > 0 && cfg.sound.trackRattle > 0 && !voices[TRACK_RATTLE].active) {
            uint32_t interval = map(virtualSpeed, 0, cfg.engine.maxRpm,
                                   cfg.features.trackRattleIntervalMax, cfg.features.trackRattleIntervalMin);
            if (now - lastTrackRattleTime > interval) {
                voices[TRACK_RATTLE].active = true;
                voices[TRACK_RATTLE].position = 0;
                lastTrackRattleTime = now;
            }
        }

        // ── Wastegate detection ──
        if (sounds.slots[WASTEGATE].samples && sounds.slots[WASTEGATE].sampleCount > 0) {
            int16_t throttleDrop = lastThrottle - throttle;
            if (throttleDrop > 150 && currentRpmFixed > 200 && !wastegateTriggered) {
                wastegateTriggered = true;
                voices[WASTEGATE].active = true;
                voices[WASTEGATE].position = 0;
                wastegateTriggerMillis = now;
            }
        }

        // ── Engine stop request ──
        if (engineStopRequested && currentRpm < 30) {
            state = STOPPING;
            engineStopRequested = false;
            stopStartMillis = now;
            stopDurationMs = cfg.engine.stopDuration > 0 ? cfg.engine.stopDuration : 1400;
            stopPitchFactor = pitchFactor;
            stopVolume = 100;
        }
    } else if (state == OFF) {
        currentRpm = 0;
        voices[IDLE].active = false;
        voices[REV].active = false;
        voices[KNOCK].active = false;
        voices[TURBO].active = false;
        voices[FAN].active = false;
        voices[SUPERCHARGER].active = false;
        voices[JAKE_BRAKE].active = false;
        voices[WASTEGATE].active = false;
        voices[TIRE_SQUEAL].active = false;
        voices[HYDRAULIC_PUMP].active = false;
        voices[HYDRAULIC_FLOW].active = false;
        voices[TRACK_RATTLE].active = false;
        voices[SHIFTING].active = false;
        voices[BRAKE].active = false;
        voices[PARKING_BRAKE].active = false;
    }

    // ── Update virtualSpeed for non-automatic transmissions ──
    if (cfg.transmission.type != TRANS_AUTOMATIC && state == RUNNING) {
        if (targetRpm > virtualSpeed) {
            virtualSpeed += max((int32_t)1, (int32_t)(cfg.engine.acc * 2));
            if (virtualSpeed > cfg.engine.maxRpm) virtualSpeed = cfg.engine.maxRpm;
        } else if (targetRpm < virtualSpeed) {
            virtualSpeed -= max((int32_t)1, (int32_t)(cfg.engine.dec * 2));
            if (virtualSpeed < 0) virtualSpeed = 0;
        }
    }

    currentRpmFixed = currentRpm;
    lastThrottle = throttle;

    // ── PARKING_BRAKE → OFF transition ──
    if (state == PARKING_BRAKE && !voices[PARKING_BRAKE].active) {
        state = OFF;
        virtualSpeed = 0;
        lastGear = 1;
    }

    // ── Compute pitch factor from RPM ──
    if (state == RUNNING) {
        pitchFactor = 1.0f + ((float)currentRpmFixed / (float)cfg.engine.maxRpm) * (cfg.engine.maxPitchFactor - 1.0f);
    } else if (state == STOPPING) {
        uint32_t elapsed = (now >= stopStartMillis) ? (now - stopStartMillis) : 0;
        float progress = (stopDurationMs > 0) ? ((float)elapsed / (float)stopDurationMs) : 1.0f;
        if (progress > 1.0f) progress = 1.0f;

        const float minPitch = 0.18f;
        float remaining = 1.0f - progress;
        stopPitchFactor = minPitch + (1.0f - minPitch) * powf(remaining, 1.2f);
        pitchFactor = stopPitchFactor;

        stopVolume = (uint8_t)(remaining * 100.0f);

        // Turbo whistle rapid fade over first 50% of stop duration
        if (voices[TURBO].active && cfg.sound.turbo > 0) {
            float turboRemaining = (progress < 0.5f) ? (1.0f - (progress * 2.0f)) : 0.0f;
            voices[TURBO].volume = (uint8_t)(cfg.sound.turbo * turboRemaining);
            if (turboRemaining <= 0.0f) voices[TURBO].active = false;
        }

        if (progress >= 1.0f || stopPitchFactor <= minPitch) {
            if (sounds.slots[PARKING_BRAKE].samples && sounds.slots[PARKING_BRAKE].sampleCount > 0) {
                state = PARKING_BRAKE;
                voices[PARKING_BRAKE].active = true;
                voices[PARKING_BRAKE].position = 0;
            } else {
                state = OFF;
            }
        }
    } else {
        pitchFactor = 1.0f;
    }

    // ── Idle/Rev cross-fade with throttle-dependent volume scaling ──
    int16_t idleProportion = 100;
    if ((state == RUNNING || state == STOPPING) && !engineMuted) {
        voices[IDLE].active = (sounds.slots[IDLE].samples && sounds.slots[IDLE].sampleCount > 0);
        voices[REV].active = (state == RUNNING) && (sounds.slots[REV].samples && sounds.slots[REV].sampleCount > 0);
        if (state == STOPPING) {
            voices[IDLE].volume = cfg.sound.idle;
            voices[REV].volume = 0;
        } else {
            if (currentRpmFixed <= cfg.engine.revSwitchPoint) {
                idleProportion = 100;
            } else if (currentRpmFixed >= cfg.engine.idleEndPoint) {
                idleProportion = 0;
            } else {
                idleProportion = map(currentRpmFixed, cfg.engine.revSwitchPoint, cfg.engine.idleEndPoint, 100, 0);
                idleProportion = constrain(idleProportion, 0, 100);
            }

            // Scale idle/rev volumes with throttle input (reference dynamic scaling)
            int32_t minEngineScale = (cfg.sound.idleMin > 0) ? cfg.sound.idleMin : 50;
            int32_t maxEngineScale = (cfg.sound.fullThrottle > 0) ? cfg.sound.fullThrottle : 150;
            int32_t throttleVol = map(throttlePercent, 0, 100, minEngineScale, maxEngineScale);
            voices[IDLE].volume = engineMuted ? 0 : (uint8_t)constrain((int32_t)cfg.sound.idle * throttleVol / 100 * idleProportion / 100, 0, 255);
            voices[REV].volume  = engineMuted ? 0 : (uint8_t)constrain((int32_t)cfg.sound.rev  * throttleVol / 100 * (100 - idleProportion) / 100, 0, 255);
        }
    } else {
        voices[IDLE].active = false;
        voices[REV].active = false;
    }

    // ── Turbo volume & active state ──
    if (state == RUNNING && !engineMuted && sounds.slots[TURBO].samples && cfg.sound.turbo > 0) {
        voices[TURBO].active = true;
        int32_t turboScale = map(currentRpmFixed, 0, cfg.engine.maxRpm, 0, 100);
        voices[TURBO].volume = (uint8_t)(cfg.sound.turboMin + (int32_t)(cfg.sound.turbo - cfg.sound.turboMin) * constrain(turboScale, 0, 100) / 100);
    } else if (state != STOPPING) {
        voices[TURBO].active = false;
        voices[TURBO].volume = 0;
    }

    // ── Fan volume & active state ──
    if (state == RUNNING && !engineMuted && sounds.slots[FAN].samples && cfg.sound.fan > 0) {
        voices[FAN].active = true;
        int32_t fanScale = map(currentRpmFixed, 0, cfg.engine.maxRpm, 0, 100);
        voices[FAN].volume = (uint8_t)(cfg.sound.fan * constrain(fanScale, 0, 100) / 100);
    } else {
        voices[FAN].active = false;
        voices[FAN].volume = 0;
    }

    // ── Supercharger: RPM-dependent with start point and min volume ──
    if (state == RUNNING && !engineMuted && sounds.slots[SUPERCHARGER].samples && cfg.sound.supercharger > 0 &&
        currentRpmFixed > (uint16_t)(cfg.engine.maxRpm * cfg.engine.superchargerStartPoint / 100)) {
        voices[SUPERCHARGER].active = true;
        int32_t scScale = map(currentRpmFixed, cfg.engine.maxRpm * cfg.engine.superchargerStartPoint / 100, cfg.engine.maxRpm, 0, 100);
        voices[SUPERCHARGER].volume = (uint8_t)(cfg.sound.superchargerMin + (int32_t)(cfg.sound.supercharger - cfg.sound.superchargerMin) * constrain(scScale, 0, 100) / 100);
    } else {
        voices[SUPERCHARGER].active = false;
        voices[SUPERCHARGER].volume = 0;
    }

    // ── Knock trigger (based on idle loop position) ──
    if ((state == RUNNING || state == STOPPING) && !engineMuted && sounds.slots[KNOCK].samples && cfg.sound.knock > 0 && cfg.engine.knockInterval > 0) {
        uint32_t totalSamples = sounds.slots[IDLE].sampleCount;
        uint32_t knockIntervalSamples = (totalSamples > 0) ? (totalSamples / cfg.engine.knockInterval) : 0;
        if (knockIntervalSamples > 0) {
            uint32_t idlePos = (uint32_t)voices[IDLE].position;
            uint32_t elapsed = (idlePos >= lastKnockTriggerSample)
                ? (idlePos - lastKnockTriggerSample)
                : (totalSamples - lastKnockTriggerSample + idlePos);

            if (elapsed >= knockIntervalSamples) {
                lastKnockTriggerSample = idlePos;
                curKnockCylinder++;
                if (curKnockCylinder > cfg.engine.knockInterval) curKnockCylinder = 1;
                voices[KNOCK].active = true;
                voices[KNOCK].position = 0;
            }
        }
    }

    // ── Knock cylinder-adaptive volume with RPM scaling ──
    if (voices[KNOCK].active && cfg.sound.knock > 0) {
        bool isLoud = false;
        switch (cfg.engine.knockPattern) {
            case KNOCK_V8:
                isLoud = (curKnockCylinder == 4 || curKnockCylinder == 8);
                break;
            case KNOCK_V8_468:
                isLoud = (curKnockCylinder == 1 || curKnockCylinder == 5 ||
                         curKnockCylinder == 9 || curKnockCylinder == 13);
                break;
            case KNOCK_R6:
                isLoud = (curKnockCylinder == 6);
                break;
            case KNOCK_R6_2:
                isLoud = (curKnockCylinder == 3 || curKnockCylinder == 6);
                break;
            case KNOCK_V2:
                isLoud = (curKnockCylinder == 1 || curKnockCylinder == 2);
                break;
            case KNOCK_UNIFORM:
            default:
                isLoud = true;
                break;
        }

        uint16_t knockRpmThreshold = (uint16_t)(cfg.engine.maxRpm * cfg.engine.knockStartRpm / 100);
        uint16_t baseKnockVol = isLoud ? cfg.sound.knock : (uint16_t)(cfg.sound.knock * cfg.engine.knockAdaptiveVolume / 100);
        uint16_t minVol = cfg.sound.knockMin;  // Direct volume value, not percentage of knock
        uint16_t minSecondary = (uint16_t)(minVol * cfg.engine.knockAdaptiveVolume / 100);

        uint16_t vol = (uint16_t)(isLoud ? minVol : minSecondary);
        if (currentRpmFixed > knockRpmThreshold) {
            uint16_t rpmScale = map(currentRpmFixed, knockRpmThreshold, cfg.engine.maxRpm,
                                   isLoud ? minVol : minSecondary, baseKnockVol);
            vol = rpmScale;
        }
        if (state == STOPPING) {
            vol = (vol * stopVolume) / 100;
        }
        voices[KNOCK].volume = (uint8_t)vol;
    }

    // ── Jake brake sound: active when jake braking ──
    voices[JAKE_BRAKE].active = jakeBrakeActive;
}

// ─── Multi-Voice Mixer with Fractional Step Interpolation ───────────────────
void RcEngineSound::renderBlock(int16_t* interleavedStereoBuffer, size_t frames) {
    if (!interleavedStereoBuffer || frames == 0) return;

    // Snapshot voices and state under single critical section
    VoiceState snapshot[SOUND_COUNT];
    uint32_t currentStartPos;
    EngineState currentState;
    float currentPitchFactor;
    uint8_t currentStopVolume;

    portENTER_CRITICAL(&voiceMutex);
    memcpy(snapshot, voices, sizeof(voices));
    currentStartPos = startPos;
    currentState = state;
    currentPitchFactor = pitchFactor;
    currentStopVolume = stopVolume;
    portEXIT_CRITICAL(&voiceMutex);

    float engineStep = currentPitchFactor;
    if (currentState == STARTING) engineStep = 1.0f;
    if (currentState == PARKING_BRAKE || currentState == OFF) engineStep = 0.0f;

    // Precalculate floating-point master, group mix weights, and per-voice gains
    float masterScale = (float)cfg.sound.master * 256.0f / 100.0f;
    float engineMixScale = (float)cfg.sound.engineMixWeight / 100.0f;
    float effectMixScale = (float)cfg.sound.effectMixWeight / 100.0f;
    float stopVolScale = (currentState == STOPPING) ? ((float)currentStopVolume / 100.0f) : 1.0f;

    float voiceGains[SOUND_COUNT] = {0.0f};
    bool voiceActive[SOUND_COUNT] = {false};
    float voiceSteps[SOUND_COUNT] = {1.0f};

    for (int i = 0; i < SOUND_COUNT; i++) {
        const VoiceState& v = snapshot[i];
        if (!v.active || !v.samples || v.count == 0) continue;
        if (currentState == PARKING_BRAKE && i != PARKING_BRAKE) continue;
        if (currentState == OFF) {
            // When engine is OFF, only standalone user effects can play
            if (i != HORN && i != SIREN && i != INDICATOR && i != BELL && 
                i != DOOR && i != SCANNER && i != MUSIC && i != WHISTLE && 
                i != GUN && i != COUPLING && i != UNCOUPLING) {
                continue;
            }
        }

        voiceActive[i] = true;
        voiceSteps[i] = v.pitchShifted ? engineStep : 1.0f;

        float groupMult = 1.0f;
        float mixWeight = v.pitchShifted ? engineMixScale : effectMixScale;

        if (v.pitchShifted) {
            if (i == TURBO || i == FAN || i == SUPERCHARGER) groupMult = 0.2f;
            else if (i == HYDRAULIC_PUMP || i == TRACK_RATTLE) groupMult = 1.0f;
            else groupMult = 0.8f; // IDLE, REV, JAKE_BRAKE
        } else {
            if (i == HORN || i == SIREN) groupMult = 0.8f;
            else if (i == KNOCK || i == WASTEGATE || i == BRAKE || 
                     i == PARKING_BRAKE || i == SHIFTING || i == REVERSING || 
                     i == COUPLING || i == UNCOUPLING) groupMult = 0.2f;
            else if (i == INDICATOR) groupMult = 0.1f;
            else groupMult = 1.0f;
        }

        float vVol = (float)v.volume / 100.0f;
        if (currentState == STOPPING && v.pitchShifted) {
            vVol *= stopVolScale;
        }

        voiceGains[i] = vVol * groupMult * mixWeight * masterScale;
    }

    float startGain = ((float)cfg.sound.start / 100.0f) * 0.8f * engineMixScale * masterScale;

    for (size_t f = 0; f < frames; f++) {
        float mixAccum = 0.0f;

        for (int i = 0; i < SOUND_COUNT; i++) {
            if (!voiceActive[i]) continue;
            VoiceState& v = snapshot[i];

            float rawSample = readInterpolatedHermite4p(v.samples, v.count, v.position, v.loop, v.loopBegin, v.loopEnd);
            mixAccum += rawSample * voiceGains[i];

            v.step = voiceSteps[i];
            advanceVoice(v);
        }

        // Start sound processing
        if (currentState == STARTING) {
            if (sounds.slots[START].samples && sounds.slots[START].sampleCount > 0) {
                if (currentStartPos < sounds.slots[START].sampleCount) {
                    float startSample = (float)sounds.slots[START].samples[currentStartPos];
                    mixAccum += startSample * startGain;
                    currentStartPos++;
                }
                if (currentStartPos >= sounds.slots[START].sampleCount) {
                    currentState = RUNNING;
                    currentStartPos = 0;
                    snapshot[IDLE].position = 0;
                    snapshot[REV].position = 0;
                    snapshot[TURBO].position = 0;
                    snapshot[KNOCK].position = 0;
                    lastKnockTriggerSample = 0;
                    curKnockCylinder = 1;
                    if (sounds.slots[BRAKE].samples && sounds.slots[BRAKE].sampleCount > 0 && cfg.sound.brake > 0) {
                        snapshot[BRAKE].active = true;
                        snapshot[BRAKE].position = 0;
                    }
                }
            } else {
                currentState = RUNNING;
                currentStartPos = 0;
            }
        }

        // Apply warm analog soft-knee saturation / limiting before 16-bit PCM output
        int16_t sample16 = saturateSoftKnee(mixAccum);

        interleavedStereoBuffer[f * 2]     = sample16;
        interleavedStereoBuffer[f * 2 + 1] = sample16;
    }

    // Write back voice states, startPos, and state under single critical section
    portENTER_CRITICAL(&voiceMutex);
    for (int i = 0; i < SOUND_COUNT; i++) {
        if (snapshot[i].samples && snapshot[i].count > 0) {
            voices[i].position = snapshot[i].position;
            voices[i].active = snapshot[i].active;
        }
    }
    startPos = currentStartPos;
    state = currentState;
    portEXIT_CRITICAL(&voiceMutex);
}

uint8_t RcEngineSound::getNextSample() {
    int16_t temp[2];
    renderBlock(temp, 1);
    int32_t u8 = (temp[0] >> 8) + 128;
    return (uint8_t)constrain(u8, 0, 255);
}
