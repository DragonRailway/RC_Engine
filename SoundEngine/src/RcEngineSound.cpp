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
    crawlerMode(false)
{
    // Initialize all voices to inactive
    for (int i = 0; i < VOICE_COUNT; i++) {
        voices[i] = VoiceState();
    }
}

RcEngineSound::~RcEngineSound() {
    if (sounds.isDynamic) {
        if (sounds.samples) free(sounds.samples);
        if (sounds.startSamples) free(sounds.startSamples);
        if (sounds.revSamples) free(sounds.revSamples);
        if (sounds.turboSamples) free(sounds.turboSamples);
        if (sounds.knockSamples) free(sounds.knockSamples);
        if (sounds.wastegateSamples) free(sounds.wastegateSamples);
        if (sounds.hornSamples) free(sounds.hornSamples);
        if (sounds.jakeBrakeSamples) free(sounds.jakeBrakeSamples);
        if (sounds.fanSamples) free(sounds.fanSamples);
        if (sounds.sirenSamples) free(sounds.sirenSamples);
        if (sounds.brakeSamples) free(sounds.brakeSamples);
        if (sounds.reversingSamples) free(sounds.reversingSamples);
        if (sounds.parkingBrakeSamples) free(sounds.parkingBrakeSamples);
        if (sounds.superchargerSamples) free(sounds.superchargerSamples);
        if (sounds.shiftingSamples) free(sounds.shiftingSamples);
        if (sounds.indicatorSamples) free(sounds.indicatorSamples);
        if (sounds.couplingSamples) free(sounds.couplingSamples);
        if (sounds.uncouplingSamples) free(sounds.uncouplingSamples);
        if (sounds.sound1Samples) free(sounds.sound1Samples);
    }
}

void RcEngineSound::begin(const SoundData& soundData, const Config& config) {
    sounds = soundData;
    cfg = config;

    // Configure engine voices (pitch-shifted)
    voices[VOICE_IDLE].samples = sounds.samples;
    voices[VOICE_IDLE].count = sounds.sampleCount;
    voices[VOICE_IDLE].pitchShifted = true;
    voices[VOICE_IDLE].loop = true;

    voices[VOICE_REV].samples = sounds.revSamples;
    voices[VOICE_REV].count = sounds.revSampleCount;
    voices[VOICE_REV].pitchShifted = true;
    voices[VOICE_REV].loop = true;

    voices[VOICE_TURBO].samples = sounds.turboSamples;
    voices[VOICE_TURBO].count = sounds.turboSampleCount;
    voices[VOICE_TURBO].pitchShifted = true;
    voices[VOICE_TURBO].loop = true;

    voices[VOICE_KNOCK].samples = sounds.knockSamples;
    voices[VOICE_KNOCK].count = sounds.knockSampleCount;
    voices[VOICE_KNOCK].pitchShifted = false; // Knock is fixed pitch
    voices[VOICE_KNOCK].loop = false;         // One-shot per trigger

    voices[VOICE_WASTEGATE].samples = sounds.wastegateSamples;
    voices[VOICE_WASTEGATE].count = sounds.wastegateSampleCount;
    voices[VOICE_WASTEGATE].pitchShifted = false;
    voices[VOICE_WASTEGATE].loop = false;

    // Horn with loop points
    voices[VOICE_HORN].samples = sounds.hornSamples;
    voices[VOICE_HORN].count = sounds.hornSampleCount;
    voices[VOICE_HORN].pitchShifted = false;
    voices[VOICE_HORN].loop = true;
    voices[VOICE_HORN].loopBegin = cfg.hornLoopBegin;
    voices[VOICE_HORN].loopEnd = cfg.hornLoopEnd;

    // Siren with loop points
    voices[VOICE_SIREN].samples = sounds.sirenSamples;
    voices[VOICE_SIREN].count = sounds.sirenSampleCount;
    voices[VOICE_SIREN].pitchShifted = false;
    voices[VOICE_SIREN].loop = true;
    voices[VOICE_SIREN].loopBegin = cfg.sirenLoopBegin;
    voices[VOICE_SIREN].loopEnd = cfg.sirenLoopEnd;

    voices[VOICE_BRAKE].samples = sounds.brakeSamples;
    voices[VOICE_BRAKE].count = sounds.brakeSampleCount;
    voices[VOICE_BRAKE].pitchShifted = false;
    voices[VOICE_BRAKE].oneShot = true;

    voices[VOICE_JAKE_BRAKE].samples = sounds.jakeBrakeSamples;
    voices[VOICE_JAKE_BRAKE].count = sounds.jakeBrakeSampleCount;
    voices[VOICE_JAKE_BRAKE].pitchShifted = true;
    voices[VOICE_JAKE_BRAKE].loop = true;

    // Reversing with loop points
    voices[VOICE_REVERSING].samples = sounds.reversingSamples;
    voices[VOICE_REVERSING].count = sounds.reversingSampleCount;
    voices[VOICE_REVERSING].pitchShifted = false;
    voices[VOICE_REVERSING].loop = true;
    voices[VOICE_REVERSING].loopBegin = cfg.reversingLoopBegin;
    voices[VOICE_REVERSING].loopEnd = cfg.reversingLoopEnd;

    voices[VOICE_PARKING_BRAKE].samples = sounds.parkingBrakeSamples;
    voices[VOICE_PARKING_BRAKE].count = sounds.parkingBrakeSampleCount;
    voices[VOICE_PARKING_BRAKE].pitchShifted = false;
    voices[VOICE_PARKING_BRAKE].oneShot = true;

    voices[VOICE_SHIFTING].samples = sounds.shiftingSamples;
    voices[VOICE_SHIFTING].count = sounds.shiftingSampleCount;
    voices[VOICE_SHIFTING].pitchShifted = false;
    voices[VOICE_SHIFTING].oneShot = true;

    voices[VOICE_INDICATOR].samples = sounds.indicatorSamples;
    voices[VOICE_INDICATOR].count = sounds.indicatorSampleCount;
    voices[VOICE_INDICATOR].pitchShifted = false;
    voices[VOICE_INDICATOR].loop = true;

    voices[VOICE_COUPLING].samples = sounds.couplingSamples;
    voices[VOICE_COUPLING].count = sounds.couplingSampleCount;
    voices[VOICE_COUPLING].pitchShifted = false;
    voices[VOICE_COUPLING].oneShot = true;

    voices[VOICE_FAN].samples = sounds.fanSamples;
    voices[VOICE_FAN].count = sounds.fanSampleCount;
    voices[VOICE_FAN].pitchShifted = true;
    voices[VOICE_FAN].loop = true;

    voices[VOICE_SUPERCHARGER].samples = sounds.superchargerSamples;
    voices[VOICE_SUPERCHARGER].count = sounds.superchargerSampleCount;
    voices[VOICE_SUPERCHARGER].pitchShifted = true;
    voices[VOICE_SUPERCHARGER].loop = true;

    voices[VOICE_UNCOUPLING].samples = sounds.uncouplingSamples;
    voices[VOICE_UNCOUPLING].count = sounds.uncouplingSampleCount;
    voices[VOICE_UNCOUPLING].pitchShifted = false;
    voices[VOICE_UNCOUPLING].oneShot = true;

    // Sound1 with loop points
    voices[VOICE_SOUND1].samples = sounds.sound1Samples;
    voices[VOICE_SOUND1].count = sounds.sound1SampleCount;
    voices[VOICE_SOUND1].pitchShifted = false;
    voices[VOICE_SOUND1].loop = true;
    voices[VOICE_SOUND1].loopBegin = cfg.sound1LoopBegin;
    voices[VOICE_SOUND1].loopEnd = cfg.sound1LoopEnd;

    // Configure default volumes
    voices[VOICE_IDLE].volume = cfg.idleVolume;
    voices[VOICE_REV].volume = cfg.revVolume;
    voices[VOICE_TURBO].volume = cfg.turboVolume;
    voices[VOICE_KNOCK].volume = cfg.knockVolume;
    voices[VOICE_WASTEGATE].volume = cfg.wastegateVolume;
    voices[VOICE_HORN].volume = cfg.hornVolume;
    voices[VOICE_SIREN].volume = cfg.sirenVolume;
    voices[VOICE_BRAKE].volume = cfg.brakeVolume;
    voices[VOICE_JAKE_BRAKE].volume = cfg.jakeBrakeVolume;
    voices[VOICE_REVERSING].volume = cfg.reversingVolume;
    voices[VOICE_PARKING_BRAKE].volume = cfg.parkingBrakeVolume;
    voices[VOICE_SHIFTING].volume = cfg.shiftingVolume;
    voices[VOICE_INDICATOR].volume = cfg.indicatorVolume;
    voices[VOICE_COUPLING].volume = cfg.couplingVolume;
    voices[VOICE_FAN].volume = cfg.fanVolume;
    voices[VOICE_SUPERCHARGER].volume = cfg.superchargerVolume;
    voices[VOICE_UNCOUPLING].volume = cfg.uncouplingVolume;
    voices[VOICE_SOUND1].volume = cfg.sound1Volume;

    Serial.printf("[RcEngineSound] Initialized with %d voice slots\n", VOICE_COUNT);
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
    voices[VOICE_HORN].active = active;
    if (active) voices[VOICE_HORN].position = 0;
}

void RcEngineSound::triggerSiren(bool active) {
    voices[VOICE_SIREN].active = active;
    if (active) voices[VOICE_SIREN].position = 0;
}

void RcEngineSound::triggerBrake(bool active) {
    voices[VOICE_BRAKE].active = active;
    if (active) voices[VOICE_BRAKE].position = 0;
}

void RcEngineSound::triggerParkingBrake(bool active) {
    voices[VOICE_PARKING_BRAKE].active = active;
    if (active) voices[VOICE_PARKING_BRAKE].position = 0;
}

void RcEngineSound::triggerReversing(bool active) {
    voices[VOICE_REVERSING].active = active;
    if (active) voices[VOICE_REVERSING].position = 0;
}

void RcEngineSound::triggerShifting(bool active) {
    voices[VOICE_SHIFTING].active = active;
    if (active) voices[VOICE_SHIFTING].position = 0;
}

void RcEngineSound::triggerIndicator(bool active) {
    voices[VOICE_INDICATOR].active = active;
    if (active) voices[VOICE_INDICATOR].position = 0;
}

void RcEngineSound::triggerCoupling(bool active) {
    voices[VOICE_COUPLING].active = active;
    if (active) voices[VOICE_COUPLING].position = 0;
}

void RcEngineSound::triggerUncoupling(bool active) {
    voices[VOICE_UNCOUPLING].active = active;
    if (active) voices[VOICE_UNCOUPLING].position = 0;
}

void RcEngineSound::triggerSound1(bool active) {
    voices[VOICE_SOUND1].active = active;
    if (active) voices[VOICE_SOUND1].position = 0;
}

// ─── Advanced Engine State Machine ───────────────────────────────────────────
void RcEngineSound::update(int16_t throttle) {
    uint32_t now = millis();
    if (lastUpdateTime == 0) lastUpdateTime = now;
    lastUpdateTime = now;

    int32_t targetRpm = abs(throttle);
    if (targetRpm > cfg.maxRpm) targetRpm = cfg.maxRpm;

    // ── Crawler Mode Detection ──
    crawlerMode = (cfg.masterVolume <= cfg.crawlerModeThreshold);

    // ── Automatic Transmission Simulation ──
    int32_t effectiveTarget = targetRpm;
    if (cfg.transmissionType == TRANS_AUTOMATIC && state == RUNNING) {
        int32_t gearSize = cfg.maxRpm / cfg.numberOfGears;
        if (gearSize > 0) {
            if (throttle > virtualSpeed) {
                virtualSpeed += max((int32_t)1, (int32_t)(cfg.acc * 2));
                if (virtualSpeed > cfg.maxRpm) virtualSpeed = cfg.maxRpm;
            } else if (throttle < virtualSpeed) {
                virtualSpeed -= max((int32_t)1, (int32_t)(cfg.dec * 2));
                if (virtualSpeed < 0) virtualSpeed = 0;
            }
            selectedGear = (uint8_t)(virtualSpeed / gearSize);
            if (selectedGear >= cfg.numberOfGears) selectedGear = cfg.numberOfGears - 1;
            int32_t gearBase = selectedGear * gearSize;
            int32_t throttleInGear = targetRpm - gearBase;
            if (throttleInGear < 0) throttleInGear = 0;
            if (throttleInGear > gearSize) throttleInGear = gearSize;
            effectiveTarget = (targetRpm < gearBase) ? targetRpm : gearBase + throttleInGear;
        }
    }

    // ── Manual Transmission Shifting Trigger ──
    if (cfg.transmissionType == TRANS_MANUAL && state == RUNNING) {
        int32_t gearSize = cfg.maxRpm / cfg.numberOfGears;
        if (gearSize > 0) {
            uint8_t newGear = (uint8_t)(targetRpm / gearSize) + 1;
            if (newGear > cfg.numberOfGears) newGear = cfg.numberOfGears;
            if (newGear != lastGear && cfg.shiftingVolume > 0) {
                voices[VOICE_SHIFTING].active = true;
                voices[VOICE_SHIFTING].position = 0;
            }
            lastGear = newGear;
            selectedGear = newGear;
        }
    }

    if (state == RUNNING) {
        // ── Jake brake auto-detection ──
        bool throttleReleased = (effectiveTarget < currentRpm);
        bool aboveJakeThreshold = (currentRpmFixed > (uint16_t)(cfg.maxRpm * cfg.jakeBrakeMinRpm / 100));
        bool throttleApplied = (effectiveTarget > currentRpm);
        
        if (throttleReleased && aboveJakeThreshold && cfg.jakeBrakeVolume > 0) {
            jakeBrakeActive = true;
            engineMuted = true;
        } else if (throttleApplied || currentRpmFixed <= (uint16_t)(cfg.maxRpm * cfg.jakeBrakeMinRpm / 100)) {
            jakeBrakeActive = false;
            engineMuted = false;
        }

        // ── Jake brake RPM deceleration ──
        if (jakeBrakeActive) {
            currentRpm -= cfg.jakeBrakeDecelRate;
            if (currentRpm < 0) currentRpm = 0;
        } else if (crawlerMode) {
            // ── Crawler mode: instant RPM response ──
            currentRpm = effectiveTarget;
        } else {
            // ── Normal RPM calculation with inertia ──
            int32_t inertiaFactor = max((int32_t)1, (int32_t)(101 - cfg.inertia));
            int32_t diff = effectiveTarget - currentRpm;

            int32_t accelStep = cfg.acc;
            int32_t decelStep = cfg.dec;
            if (cfg.transmissionType == TRANS_AUTOMATIC && selectedGear < 6) {
                accelStep = cfg.gearRampTimes[selectedGear];
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

        // ── Wastegate detection ──
        if (sounds.wastegateSamples && sounds.wastegateSampleCount > 0) {
            int16_t throttleDrop = lastThrottle - throttle;
            if (throttleDrop > 150 && currentRpmFixed > 200 && !wastegateTriggered) {
                wastegateTriggered = true;
                voices[VOICE_WASTEGATE].active = true;
                voices[VOICE_WASTEGATE].position = 0;
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

    currentRpmFixed = currentRpm;
    lastThrottle = throttle;

    // ── PARKING_BRAKE → OFF transition ──
    if (state == PARKING_BRAKE && !voices[VOICE_PARKING_BRAKE].active) {
        state = OFF;
        virtualSpeed = 0;
        lastGear = 1;
    }

    // ── Compute pitch factor from RPM ──
    if (state == RUNNING) {
        pitchFactor = 1.0f + ((float)currentRpmFixed / (float)cfg.maxRpm) * (cfg.maxPitchFactor - 1.0f);
    } else if (state == STOPPING) {
        if (now - attenuatorMillis > 80) {
            stopPitchFactor -= 0.05f;
            if (stopPitchFactor < 1.0f) stopPitchFactor = 1.0f;
            attenuator++;
            attenuatorMillis = now;
        }
        pitchFactor = stopPitchFactor;
        if (attenuator >= 40) {
            if (sounds.parkingBrakeSamples && sounds.parkingBrakeSampleCount > 0) {
                state = PARKING_BRAKE;
                voices[VOICE_PARKING_BRAKE].active = true;
                voices[VOICE_PARKING_BRAKE].position = 0;
            } else {
                state = OFF;
            }
        }
    } else if (state == STARTING) {
        pitchFactor = 1.0f;
    } else {
        pitchFactor = 1.0f;
    }

    // ── Idle/Rev cross-fade proportion ──
    int16_t idleProportion = 100;
    if (state == RUNNING && !engineMuted) {
        if (currentRpmFixed > cfg.revSwitchPoint) {
            idleProportion = map(currentRpmFixed, cfg.idleEndPoint, cfg.revSwitchPoint, 0, 100);
            idleProportion = constrain(idleProportion, 0, 100);
        }
    }
    voices[VOICE_IDLE].volume = engineMuted ? 0 : (uint8_t)((int32_t)cfg.idleVolume * idleProportion / 100);
    voices[VOICE_REV].volume = engineMuted ? 0 : (uint8_t)((int32_t)cfg.revVolume * (100 - idleProportion) / 100);

    // ── Turbo volume: RPM-dependent ──
    if (!engineMuted) {
        int32_t turboScale = map(currentRpmFixed, 0, cfg.maxRpm, 0, 100);
        voices[VOICE_TURBO].volume = (uint8_t)(cfg.turboVolume * constrain(turboScale, 0, 100) / 100);
    } else {
        voices[VOICE_TURBO].volume = 0;
    }

    // ── Fan volume: RPM-dependent ──
    if (!engineMuted) {
        int32_t fanScale = map(currentRpmFixed, 0, cfg.maxRpm, 0, 100);
        voices[VOICE_FAN].volume = (uint8_t)(cfg.fanVolume * constrain(fanScale, 0, 100) / 100);
    } else {
        voices[VOICE_FAN].volume = 0;
    }

    // ── Supercharger: RPM-dependent with start point ──
    if (!engineMuted && currentRpmFixed > (uint16_t)(cfg.maxRpm * cfg.superchargerStartPoint / 100)) {
        int32_t scScale = map(currentRpmFixed, cfg.maxRpm * cfg.superchargerStartPoint / 100, cfg.maxRpm, 0, 100);
        voices[VOICE_SUPERCHARGER].volume = (uint8_t)(cfg.superchargerVolume * constrain(scScale, 0, 100) / 100);
    } else {
        voices[VOICE_SUPERCHARGER].volume = 0;
    }

    // ── Knock trigger (based on idle loop position) ──
    if (state == RUNNING && !engineMuted && sounds.knockSamples && cfg.knockVolume > 0 && cfg.knockInterval > 0) {
        uint32_t knockIntervalSamples = sounds.sampleCount / cfg.knockInterval;
        if (knockIntervalSamples > 0) {
            uint32_t idlePos = (uint32_t)voices[VOICE_IDLE].position;
            if (idlePos - lastKnockTriggerSample >= knockIntervalSamples) {
                lastKnockTriggerSample = idlePos;
                curKnockCylinder++;
                if (curKnockCylinder > cfg.knockInterval) curKnockCylinder = 1;
                voices[VOICE_KNOCK].active = true;
                voices[VOICE_KNOCK].position = 0;
            }
        }
    }

    // ── Knock cylinder-adaptive volume with RPM scaling ──
    if (voices[VOICE_KNOCK].active && cfg.knockVolume > 0) {
        bool isLoud = false;
        switch (cfg.knockPattern) {
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

        // RPM-dependent knock volume scaling
        uint16_t knockRpmThreshold = (uint16_t)(cfg.maxRpm * cfg.knockStartRpm / 100);
        uint16_t baseKnockVol = isLoud ? cfg.knockVolume : (uint16_t)(cfg.knockVolume * cfg.knockAdaptiveVolume / 100);
        uint16_t minVol = (uint16_t)(cfg.knockVolume * cfg.minKnockVolume / 100);
        uint16_t minSecondary = (uint16_t)(minVol * cfg.knockAdaptiveVolume / 100);

        if (currentRpmFixed > knockRpmThreshold) {
            uint16_t rpmScale = map(currentRpmFixed, knockRpmThreshold, cfg.maxRpm, 
                                   isLoud ? minVol : minSecondary, baseKnockVol);
            voices[VOICE_KNOCK].volume = (uint8_t)rpmScale;
        } else {
            voices[VOICE_KNOCK].volume = (uint8_t)(isLoud ? minVol : minSecondary);
        }
    }

    // ── Jake brake sound: active when jake braking ──
    voices[VOICE_JAKE_BRAKE].active = jakeBrakeActive;
}

// ─── Multi-Voice Mixer with Fractional Step Interpolation ───────────────────
uint8_t RcEngineSound::getNextSample() {
    int32_t engineMix = 0;
    int32_t effectMix = 0;

    // ── Determine current pitch step ──
    float engineStep = pitchFactor;
    if (state == STARTING) engineStep = 1.0f;
    if (state == PARKING_BRAKE || state == OFF) engineStep = 0.0f;

    // ── Process each voice ──
    for (int i = 0; i < VOICE_COUNT; i++) {
        VoiceState& v = voices[i];
        if (!v.active || !v.samples || v.count == 0) continue;

        float step = v.pitchShifted ? engineStep : 1.0f;
        if (state == PARKING_BRAKE && i != VOICE_PARKING_BRAKE) continue;

        int8_t sample = readInterpolated(v.samples, v.count, v.position);
        int32_t scaled = (int32_t)sample * v.volume / 100;

        if (state == STOPPING && v.pitchShifted) {
            scaled = scaled / attenuator;
        }

        v.step = step;
        advanceVoice(v);

        // Separate engine and effect mixing
        if (v.pitchShifted) {
            engineMix += scaled;
        } else {
            effectMix += scaled;
        }
    }

    // ── Start sound (special: separate sample array, fixed rate during cranking) ──
    if (state == STARTING && sounds.startSamples && sounds.startSampleCount > 0) {
        uint32_t pos = (uint32_t)startPos;
        if (pos < sounds.startSampleCount) {
            int8_t sample = sounds.startSamples[pos];
            int32_t scaled = (int32_t)sample * cfg.startVolume / 100;
            engineMix += scaled;
            startPos++;
            if (startPos >= sounds.startSampleCount) {
                state = RUNNING;
                startPos = 0;
                voices[VOICE_IDLE].position = 0;
                voices[VOICE_REV].position = 0;
                voices[VOICE_TURBO].position = 0;
                voices[VOICE_KNOCK].position = 0;
                lastKnockTriggerSample = 0;
                curKnockCylinder = 1;
            }
        }
    }

    // ── Apply voice mixing weights ──
    int32_t mixed = (engineMix * cfg.engineMixWeight / 100) + (effectMix * cfg.effectMixWeight / 100);

    // ── Final mix: apply master volume, add DC offset ──
    mixed = (mixed * cfg.masterVolume / 100) + 128;

    return (uint8_t)constrain(mixed, 0, 255);
}
