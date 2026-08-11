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
    attenuatorMillis(0),
    attenuator(1),
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
    sounds = soundData;
    cfg = config;

    struct VoiceDef { bool pitchShifted; bool loop; bool oneShot; };
    static const VoiceDef voiceDefs[SOUND_COUNT] = {
        {true,  true,  false}, // IDLE
        {true,  true,  false}, // REV
        {false, false, false}, // START (handled separately)
        {false, false, true},  // KNOCK
        {false, true,  false}, // TURBO
        {false, false, true},  // WASTEGATE
        {false, true,  false}, // HORN
        {false, true,  false}, // SIREN
        {false, false, true},  // BRAKE
        {true,  true,  false}, // JAKE_BRAKE
        {false, true,  false}, // REVERSING
        {false, false, true},  // PARKING_BRAKE
        {false, false, true},  // SHIFTING
        {false, true,  false}, // INDICATOR
        {false, false, true},  // COUPLING
        {true,  true,  false}, // FAN
        {true,  true,  false}, // SUPERCHARGER
        {false, false, true},  // UNCOUPLING
        {false, true,  false}, // SOUND1
        {false, true,  false}, // TIRE_SQUEAL
        {true,  true,  false}, // HYDRAULIC_PUMP
        {false, true,  false}, // HYDRAULIC_FLOW
        {false, false, true},  // TRACK_RATTLE
        {false, false, true},  // BUCKET_RATTLE
        {false, true,  false}, // BELL
        {false, false, true},  // DOOR
        {false, true,  false}, // SCANNER
        {false, true,  false}, // MUSIC
        {false, true,  false}, // WHISTLE
        {false, false, true},  // GUN
        {false, true,  false}, // OUT_OF_FUEL
        {false, true,  false}, // OTHERS
    };

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

    Serial.printf("[RcEngineSound] Initialized with %d voice slots\n", SOUND_COUNT);
}

void RcEngineSound::begin(const SoundData& soundData) {
    begin(soundData, Config());
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
    if (lastUpdateTime == 0) lastUpdateTime = now;
    lastUpdateTime = now;

    int32_t targetRpm = abs(throttle);
    if (targetRpm > cfg.engine.maxRpm) targetRpm = cfg.engine.maxRpm;

    // ── Throttle ratio (0-100%) for volume scaling ──
    int32_t throttlePercent = map(targetRpm, 0, cfg.engine.maxRpm, 0, 100);

    // ── Crawler Mode Detection ──
    crawlerMode = (cfg.sound.master <= cfg.sound.crawlerModeThreshold);

    // ── Automatic Transmission Simulation ──
    int32_t effectiveTarget = targetRpm;
    if (cfg.transmission.type == TRANS_AUTOMATIC && state == RUNNING) {
        int32_t gearSize = cfg.engine.maxRpm / cfg.transmission.numberOfGears;
        if (gearSize > 0) {
            if (throttle > virtualSpeed) {
                virtualSpeed += max((int32_t)1, (int32_t)(cfg.engine.acc * 2));
                if (virtualSpeed > cfg.engine.maxRpm) virtualSpeed = cfg.engine.maxRpm;
            } else if (throttle < virtualSpeed) {
                virtualSpeed -= max((int32_t)1, (int32_t)(cfg.engine.dec * 2));
                if (virtualSpeed < 0) virtualSpeed = 0;
            }
            selectedGear = (uint8_t)(virtualSpeed / gearSize);
            if (selectedGear >= cfg.transmission.numberOfGears) selectedGear = cfg.transmission.numberOfGears - 1;
            int32_t gearBase = selectedGear * gearSize;
            int32_t throttleInGear = targetRpm - gearBase;
            if (throttleInGear < 0) throttleInGear = 0;
            if (throttleInGear > gearSize) throttleInGear = gearSize;
            effectiveTarget = (targetRpm < gearBase) ? targetRpm : gearBase + throttleInGear;
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
        bool throttleReleased = (effectiveTarget < currentRpm);
        bool aboveJakeThreshold = (currentRpmFixed > (uint16_t)(cfg.engine.maxRpm * cfg.engine.jakeBrakeMinRpm / 100));
        bool throttleApplied = (effectiveTarget > currentRpm);

        if (throttleReleased && aboveJakeThreshold && cfg.sound.jakeBrake > 0) {
            jakeBrakeActive = true;
            engineMuted = true;
        } else if (throttleApplied || currentRpmFixed <= (uint16_t)(cfg.engine.maxRpm * cfg.engine.jakeBrakeMinRpm / 100)) {
            jakeBrakeActive = false;
            engineMuted = false;
        }

        // ── Jake brake RPM deceleration ──
        if (jakeBrakeActive) {
            currentRpm -= cfg.engine.jakeBrakeDecelRate;
            if (currentRpm < 0) currentRpm = 0;
        } else if (crawlerMode) {
            currentRpm = effectiveTarget;
        } else {
            // ── Normal RPM calculation with inertia ──
            int32_t inertiaFactor = max((int32_t)1, (int32_t)(101 - cfg.engine.inertia));
            int32_t diff = effectiveTarget - currentRpm;

            int32_t accelStep = cfg.engine.acc;
            int32_t decelStep = cfg.engine.dec;
            if (cfg.transmission.type == TRANS_AUTOMATIC && selectedGear < 6) {
                accelStep = cfg.transmission.gearRampTimes[selectedGear];
                decelStep = accelStep;
            }

            if (diff > 0) {
                int32_t step = max((int32_t)1, (int32_t)((diff * inertiaFactor) / 200 + accelStep));
                currentRpm = min(currentRpm + step, effectiveTarget);
            } else if (diff < 0) {
                int32_t step = max((int32_t)1, (int32_t)(((-diff) * inertiaFactor) / 200 + decelStep));
                currentRpm = max(currentRpm - step, effectiveTarget);
            }
        }

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
            attenuator = 1;
            attenuatorMillis = now;
            stopPitchFactor = pitchFactor;
        }
    } else if (state == OFF) {
        currentRpm = 0;
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
        if (now - attenuatorMillis > 80) {
            stopPitchFactor -= 0.05f;
            if (stopPitchFactor < 1.0f) stopPitchFactor = 1.0f;
            attenuator++;
            attenuatorMillis = now;
        }
        pitchFactor = stopPitchFactor;
        if (attenuator >= 40) {
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
    if (state == RUNNING && !engineMuted) {
        if (currentRpmFixed > cfg.engine.revSwitchPoint) {
            idleProportion = map(currentRpmFixed, cfg.engine.idleEndPoint, cfg.engine.revSwitchPoint, 0, 100);
            idleProportion = constrain(idleProportion, 0, 100);
        }
    }
    // Scale idle/rev volumes with throttle input (reference parity)
    // idleVol interpolates from idle (at 0% throttle) to idleMin (at 100% throttle)
    int32_t idleVol = cfg.sound.idleMin + (int32_t)(cfg.sound.idle - cfg.sound.idleMin) * (100 - throttlePercent) / 100;
    int32_t revVol = cfg.sound.revMin + (int32_t)(cfg.sound.rev - cfg.sound.revMin) * throttlePercent / 100;
    voices[IDLE].volume = engineMuted ? 0 : (uint8_t)(idleVol * idleProportion / 100 * cfg.sound.fullThrottle / 100);
    voices[REV].volume = engineMuted ? 0 : (uint8_t)(revVol * (100 - idleProportion) / 100 * cfg.sound.fullThrottle / 100);

    // ── Turbo volume: RPM-dependent with min volume ──
    if (!engineMuted) {
        int32_t turboScale = map(currentRpmFixed, 0, cfg.engine.maxRpm, 0, 100);
        voices[TURBO].volume = (uint8_t)(cfg.sound.turboMin + (int32_t)(cfg.sound.turbo - cfg.sound.turboMin) * constrain(turboScale, 0, 100) / 100);
    } else {
        voices[TURBO].volume = 0;
    }

    // ── Fan volume: RPM-dependent ──
    if (!engineMuted) {
        int32_t fanScale = map(currentRpmFixed, 0, cfg.engine.maxRpm, 0, 100);
        voices[FAN].volume = (uint8_t)(cfg.sound.fan * constrain(fanScale, 0, 100) / 100);
    } else {
        voices[FAN].volume = 0;
    }

    // ── Supercharger: RPM-dependent with start point and min volume ──
    if (!engineMuted && currentRpmFixed > (uint16_t)(cfg.engine.maxRpm * cfg.engine.superchargerStartPoint / 100)) {
        int32_t scScale = map(currentRpmFixed, cfg.engine.maxRpm * cfg.engine.superchargerStartPoint / 100, cfg.engine.maxRpm, 0, 100);
        voices[SUPERCHARGER].volume = (uint8_t)(cfg.sound.superchargerMin + (int32_t)(cfg.sound.supercharger - cfg.sound.superchargerMin) * constrain(scScale, 0, 100) / 100);
    } else {
        voices[SUPERCHARGER].volume = 0;
    }

    // ── Knock trigger (based on idle loop position) ──
    if (state == RUNNING && !engineMuted && sounds.slots[KNOCK].samples && cfg.sound.knock > 0 && cfg.engine.knockInterval > 0) {
        uint32_t knockIntervalSamples = sounds.slots[IDLE].sampleCount / cfg.engine.knockInterval;
        if (knockIntervalSamples > 0) {
            uint32_t idlePos = (uint32_t)voices[IDLE].position;
            if (idlePos >= lastKnockTriggerSample + knockIntervalSamples) {
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

        if (currentRpmFixed > knockRpmThreshold) {
            uint16_t rpmScale = map(currentRpmFixed, knockRpmThreshold, cfg.engine.maxRpm,
                                   isLoud ? minVol : minSecondary, baseKnockVol);
            voices[KNOCK].volume = (uint8_t)rpmScale;
        } else {
            voices[KNOCK].volume = (uint8_t)(isLoud ? minVol : minSecondary);
        }
    }

    // ── Jake brake sound: active when jake braking ──
    voices[JAKE_BRAKE].active = jakeBrakeActive;
}

// ─── Multi-Voice Mixer with Fractional Step Interpolation ───────────────────
uint8_t RcEngineSound::getNextSample() {
    int32_t engineMix = 0;
    int32_t effectMix = 0;

    VoiceState snapshot[SOUND_COUNT];
    portENTER_CRITICAL(&voiceMutex);
    memcpy(snapshot, voices, sizeof(voices));
    portEXIT_CRITICAL(&voiceMutex);

    float engineStep = pitchFactor;
    if (state == STARTING) engineStep = 1.0f;
    if (state == PARKING_BRAKE || state == OFF) engineStep = 0.0f;

    for (int i = 0; i < SOUND_COUNT; i++) {
        VoiceState& v = snapshot[i];
        if (!v.active || !v.samples || v.count == 0) continue;

        float step = v.pitchShifted ? engineStep : 1.0f;
        if (state == PARKING_BRAKE && i != PARKING_BRAKE) continue;

        int8_t sample = readInterpolated(v.samples, v.count, v.position);
        int32_t scaled = (int32_t)sample * v.volume / 100;

        if (state == STOPPING && v.pitchShifted) {
            scaled = scaled / attenuator;
        }

        v.step = step;
        advanceVoice(v);

        // Write back position and active flag
        portENTER_CRITICAL(&voiceMutex);
        voices[i].position = v.position;
        voices[i].active = v.active;
        portEXIT_CRITICAL(&voiceMutex);

        if (v.pitchShifted) {
            engineMix += scaled;
        } else {
            effectMix += scaled;
        }
    }

    // ── Start sound ──
    if (state == STARTING && sounds.slots[START].samples && sounds.slots[START].sampleCount > 0) {
        uint32_t pos = (uint32_t)startPos;
        if (pos < sounds.slots[START].sampleCount) {
            int8_t sample = sounds.slots[START].samples[pos];
            int32_t scaled = (int32_t)sample * cfg.sound.start / 100;
            engineMix += scaled;
            startPos++;
            if (startPos >= sounds.slots[START].sampleCount) {
                state = RUNNING;
                startPos = 0;
                voices[IDLE].position = 0;
                voices[REV].position = 0;
                voices[TURBO].position = 0;
                voices[KNOCK].position = 0;
                lastKnockTriggerSample = 0;
                curKnockCylinder = 1;
            }
        }
    }

    // ── Apply voice mixing weights ──
    int32_t mixed = (engineMix * cfg.sound.engineMixWeight / 100) + (effectMix * cfg.sound.effectMixWeight / 100);

    // ── Final mix: apply master volume, add DC offset ──
    mixed = (mixed * cfg.sound.master / 100) + 128;

    return (uint8_t)constrain(mixed, 0, 255);
}
