#include "SoundSynth.h"
#include <cmath>

SoundSynth::SoundSynth() :
    engineState(EngineSim::OFF),
    currentRpm(0),
    pitchFactor(1.0f),
    currentThrottleFaded(0.0f),
    jakeBrakeActive(false),
    engineMuted(false),
    startPos(0),
    lastKnockTriggerSample(0),
    curKnockCylinder(0)
{
    for (int i = 0; i < SOUND_COUNT; i++) {
        voices[i] = VoiceState();
    }
}

SoundSynth::~SoundSynth() {
    if (sounds.isDynamic) {
        for (int i = 0; i < SOUND_COUNT; i++) {
            if (sounds.slots[i].samples) free(sounds.slots[i].samples);
        }
    }
}

void SoundSynth::begin(const SoundData& soundData, const Config& config) {
    sounds.clear();
    sounds = soundData;
    setConfig(config);
    currentRpm = 0;
    pitchFactor = 1.0f;
    startPos = 0;
    engineState = EngineSim::OFF;

    struct VoiceDef { bool pitchShifted; bool loop; bool oneShot; };
    VoiceDef voiceDefs[SOUND_COUNT] = {};
    voiceDefs[IDLE]           = {true,  true,  false};
    voiceDefs[REV]            = {true,  true,  false};
    voiceDefs[START]          = {false, false, false};
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

    voices[HORN].loopBegin = cfg.loopPoints.hornBegin;
    voices[HORN].loopEnd = cfg.loopPoints.hornEnd;
    voices[SIREN].loopBegin = cfg.loopPoints.sirenBegin;
    voices[SIREN].loopEnd = cfg.loopPoints.sirenEnd;
    voices[REVERSING].loopBegin = cfg.loopPoints.reversingBegin;
    voices[REVERSING].loopEnd = cfg.loopPoints.reversingEnd;
    voices[SOUND1].loopBegin = cfg.loopPoints.sound1Begin;
    voices[SOUND1].loopEnd = cfg.loopPoints.sound1End;

    applyVoiceVolumes();
    Serial.printf("[SoundSynth] Initialized with %d voice slots\n", SOUND_COUNT);
}

void SoundSynth::begin(const SoundData& soundData) {
    begin(soundData, cfg);
}

void SoundSynth::setConfig(const Config& config) {
    cfg = config;
    applyVoiceVolumes();
}

void SoundSynth::applyVoiceVolumes() {
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

void SoundSynth::syncState(const EngineSim& sim) {
    portENTER_CRITICAL(&voiceMutex);
    engineState = sim.getState();
    currentRpm = sim.getRpm();
    pitchFactor = sim.getPitchFactor();
    currentThrottleFaded = sim.getCurrentThrottleFaded();
    jakeBrakeRequest = sim.isJakeBrakeActive();
    if (jakeBrakeRequest && cfg.sound.jakeBrake > 0 && sounds.slots[JAKE_BRAKE].samples && sounds.slots[JAKE_BRAKE].sampleCount > 0) {
        jakeBrakeActive = true;
        engineMuted = true;
    } else if (!jakeBrakeRequest) {
        jakeBrakeActive = false;
        engineMuted = false;
    }

    if (sim.isWastegateTriggered() && sounds.slots[WASTEGATE].samples && sounds.slots[WASTEGATE].sampleCount > 0) {
        voices[WASTEGATE].active = true;
        voices[WASTEGATE].position = 0;
    }

    if (sim.isGearShiftTriggered() && cfg.sound.shifting > 0) {
        voices[SHIFTING].active = true;
        voices[SHIFTING].position = 0;
    }

    if (sim.isBrakeSquealTriggered() && cfg.sound.brake > 0) {
        voices[BRAKE].active = true;
        voices[BRAKE].position = 0;
    }

    // Tire Squeal
    voices[TIRE_SQUEAL].active = sim.isTireSquealActive();
    voices[TIRE_SQUEAL].volume = sim.getTireSquealVolume();

    // Hydraulic Pump
    voices[HYDRAULIC_PUMP].active = sim.isHydraulicPumpActive();
    voices[HYDRAULIC_PUMP].volume = sim.getHydraulicPumpVolume();

    // Track Rattle
    if (sim.isTrackRattleTriggered()) {
        voices[TRACK_RATTLE].active = true;
        voices[TRACK_RATTLE].position = 0;
    }

    // Reversing
    voices[REVERSING].active = sim.isReverseActive();

    // ── Reference Layered IDLE/REV Volume Calculation ──
    int32_t throttlePct = (int32_t)currentThrottleFaded;
    if (engineState == EngineSim::RUNNING || engineState == EngineSim::STOPPING) {
        voices[IDLE].active = (sounds.slots[IDLE].samples && sounds.slots[IDLE].sampleCount > 0);
        voices[REV].active = (engineState == EngineSim::RUNNING) && (sounds.slots[REV].samples && sounds.slots[REV].sampleCount > 0);
        if (engineState == EngineSim::STOPPING) {
            voices[IDLE].volume = cfg.sound.idle;
            voices[REV].volume = 0;
        } else {
            int32_t idleScale = map(throttlePct, 0, 100, cfg.sound.engineIdle, cfg.sound.fullThrottle);
            int32_t revScale  = map(throttlePct, 0, 100, cfg.sound.engineRev,  cfg.sound.fullThrottle);

            int32_t idleVol = (int32_t)((float)cfg.sound.idle * (float)idleScale / 100.0f);
            int32_t revVol  = (int32_t)((float)cfg.sound.rev  * (float)revScale  / 100.0f);

            if (cfg.sound.idleMin > 0 && idleVol < (int32_t)cfg.sound.idleMin) {
                idleVol = cfg.sound.idleMin;
            }
            if (cfg.sound.revMin > 0 && revVol < (int32_t)cfg.sound.revMin) {
                revVol = cfg.sound.revMin;
            }

            if (engineMuted) {
                idleVol = 0;
                revVol = 0;
            }

            voices[IDLE].volume = (uint8_t)constrain(idleVol, 0, 255);
            voices[REV].volume  = (uint8_t)constrain(revVol, 0, 255);
        }
    } else {
        voices[IDLE].active = false;
        voices[REV].active = false;
    }

    // Turbo & Supercharger
    if (cfg.sound.turbo > 0) {
        int32_t turboVol = cfg.sound.turboMin +
            ((int32_t)(cfg.sound.turbo - cfg.sound.turboMin) * throttlePct / 100);
        voices[TURBO].volume = (uint8_t)constrain(turboVol, 0, 100);
        voices[TURBO].active = (voices[TURBO].volume > 0 && engineState == EngineSim::RUNNING);
    }
    if (cfg.sound.supercharger > 0) {
        if (currentRpm >= (uint16_t)(cfg.engine.maxRpm * cfg.engine.superchargerStartPoint / 100)) {
            int32_t scVol = cfg.sound.superchargerMin +
                ((cfg.sound.supercharger - cfg.sound.superchargerMin) * throttlePct / 100);
            voices[SUPERCHARGER].volume = (uint8_t)constrain(scVol, 0, 100);
            voices[SUPERCHARGER].active = true;
        } else {
            voices[SUPERCHARGER].active = false;
        }
    }

    // ── Knock trigger (based on idle loop position) ──
    if ((engineState == EngineSim::RUNNING || engineState == EngineSim::STOPPING) && !engineMuted &&
        sounds.slots[KNOCK].samples && cfg.sound.knock > 0 && cfg.engine.knockInterval > 0) {
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
        uint16_t minVol = cfg.sound.knockMin;
        uint16_t minSecondary = (uint16_t)(minVol * cfg.engine.knockAdaptiveVolume / 100);

        uint16_t vol = (uint16_t)(isLoud ? minVol : minSecondary);
        if (currentRpm > knockRpmThreshold && cfg.engine.maxRpm > knockRpmThreshold) {
            uint16_t rpmScale = map(currentRpm, knockRpmThreshold, cfg.engine.maxRpm,
                                   isLoud ? minVol : minSecondary, baseKnockVol);
            vol = rpmScale;
        }
        voices[KNOCK].volume = (uint8_t)vol;
    }

    voices[JAKE_BRAKE].active = jakeBrakeActive;

    if (engineState == EngineSim::OFF) {
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
    portEXIT_CRITICAL(&voiceMutex);
}

// ── Trigger Methods ──
void SoundSynth::triggerHorn(bool active) {
    voices[HORN].active = active;
    if (active) voices[HORN].position = 0;
}
void SoundSynth::triggerSiren(bool active) {
    voices[SIREN].active = active;
    if (active) voices[SIREN].position = 0;
}
void SoundSynth::triggerBrake(bool active) {
    voices[BRAKE].active = active;
    if (active) voices[BRAKE].position = 0;
}
void SoundSynth::triggerParkingBrake(bool active) {
    voices[PARKING_BRAKE].active = active;
    if (active) voices[PARKING_BRAKE].position = 0;
}
void SoundSynth::triggerJakeBrake(bool active) {
    if (sounds.slots[JAKE_BRAKE].samples && sounds.slots[JAKE_BRAKE].sampleCount > 0 && cfg.sound.jakeBrake > 0) {
        voices[JAKE_BRAKE].active = active;
        if (active) voices[JAKE_BRAKE].position = 0;
        engineMuted = active;
    } else {
        voices[JAKE_BRAKE].active = false;
        engineMuted = false;
    }
}
void SoundSynth::triggerWastegate(bool active) {
    voices[WASTEGATE].active = active;
    if (active) voices[WASTEGATE].position = 0;
}
void SoundSynth::triggerReversing(bool active) {
    voices[REVERSING].active = active;
    if (active) voices[REVERSING].position = 0;
}
void SoundSynth::triggerShifting(bool active) {
    voices[SHIFTING].active = active;
    if (active) voices[SHIFTING].position = 0;
}
void SoundSynth::triggerIndicator(bool active) {
    voices[INDICATOR].active = active;
    if (active) voices[INDICATOR].position = 0;
}
void SoundSynth::triggerCoupling(bool active) {
    voices[COUPLING].active = active;
    if (active) voices[COUPLING].position = 0;
}
void SoundSynth::triggerUncoupling(bool active) {
    voices[UNCOUPLING].active = active;
    if (active) voices[UNCOUPLING].position = 0;
}
void SoundSynth::triggerSound1(bool active) {
    voices[SOUND1].active = active;
    if (active) voices[SOUND1].position = 0;
}
void SoundSynth::triggerTireSqueal(bool active) {
    voices[TIRE_SQUEAL].active = active;
    if (active) voices[TIRE_SQUEAL].position = 0;
}
void SoundSynth::triggerHydraulicPump(bool active) {
    voices[HYDRAULIC_PUMP].active = active;
    if (active) voices[HYDRAULIC_PUMP].position = 0;
}
void SoundSynth::triggerHydraulicFlow(bool active) {
    voices[HYDRAULIC_FLOW].active = active;
    if (active) voices[HYDRAULIC_FLOW].position = 0;
}
void SoundSynth::triggerTrackRattle(bool active) {
    voices[TRACK_RATTLE].active = active;
    if (active) voices[TRACK_RATTLE].position = 0;
}
void SoundSynth::triggerBucketRattle(bool active) {
    voices[BUCKET_RATTLE].active = active;
    if (active) voices[BUCKET_RATTLE].position = 0;
}
void SoundSynth::triggerDumpBed(bool active) {
    voices[HYDRAULIC_PUMP].active = active;
    voices[HYDRAULIC_FLOW].active = active;
    if (active) {
        voices[HYDRAULIC_PUMP].position = 0;
        voices[HYDRAULIC_FLOW].position = 0;
    }
}
void SoundSynth::triggerBell(bool active) {
    voices[BELL].active = active;
    if (active) voices[BELL].position = 0;
}
void SoundSynth::triggerDoor(bool active) {
    voices[DOOR].active = active;
    if (active) voices[DOOR].position = 0;
}
void SoundSynth::triggerScanner(bool active) {
    voices[SCANNER].active = active;
    if (active) voices[SCANNER].position = 0;
}
void SoundSynth::triggerMusic(bool active) {
    voices[MUSIC].active = active;
    if (active) voices[MUSIC].position = 0;
}
void SoundSynth::triggerWhistle(bool active) {
    voices[WHISTLE].active = active;
    if (active) voices[WHISTLE].position = 0;
}
void SoundSynth::triggerGun(bool active) {
    voices[GUN].active = active;
    if (active) voices[GUN].position = 0;
}
void SoundSynth::triggerOutOfFuel(bool active) {
    voices[OUT_OF_FUEL].active = active;
    if (active) voices[OUT_OF_FUEL].position = 0;
}
void SoundSynth::triggerOthers(bool active) {
    voices[OTHERS].active = active;
    if (active) voices[OTHERS].position = 0;
}

// ── Audio Block Rendering (22.05 kHz audioTask on Core 1) ──
void SoundSynth::renderBlock(int16_t* interleavedStereoBuffer, size_t frames) {
    if (!interleavedStereoBuffer || frames == 0) return;

    // Snapshot voices and state under single critical section
    VoiceState snapshot[SOUND_COUNT];
    uint32_t currentStartPos;
    EngineSim::State currentState;
    float currentPitchFactor;

    portENTER_CRITICAL(&voiceMutex);
    memcpy(snapshot, voices, sizeof(voices));
    currentStartPos = startPos;
    currentState = engineState;
    currentPitchFactor = pitchFactor;
    portEXIT_CRITICAL(&voiceMutex);

    float engineStep = currentPitchFactor;
    if (currentState == EngineSim::STARTING) engineStep = 1.0f;
    if (currentState == EngineSim::PARKING_BRAKE || currentState == EngineSim::OFF) engineStep = 0.0f;

    // Precalculate floating-point master, group mix weights, and per-voice gains
    float masterScale = (float)cfg.sound.master * 256.0f / 100.0f;
    float engineMixScale = (float)cfg.sound.engineMixWeight / 100.0f;
    float effectMixScale = (float)cfg.sound.effectMixWeight / 100.0f;

    float voiceGains[SOUND_COUNT] = {0.0f};
    bool voiceActive[SOUND_COUNT] = {false};
    float voiceSteps[SOUND_COUNT] = {1.0f};

    for (int i = 0; i < SOUND_COUNT; i++) {
        const VoiceState& v = snapshot[i];
        if (!v.active || !v.samples || v.count == 0) continue;
        if (currentState == EngineSim::PARKING_BRAKE && i != PARKING_BRAKE) continue;
        if (currentState == EngineSim::OFF) {
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
        voiceGains[i] = vVol * groupMult * mixWeight * masterScale;
    }

    float startGain = ((float)cfg.sound.start / 100.0f) * 0.8f * engineMixScale * masterScale;

    for (size_t f = 0; f < frames; f++) {
        float mixAccum = 0.0f;

        // Phase-lock REV to IDLE when both voices are active
        if (voiceActive[IDLE] && voiceActive[REV] && snapshot[IDLE].count > 0 && snapshot[REV].count > 0) {
            float phase = snapshot[IDLE].position / (float)snapshot[IDLE].count;
            snapshot[REV].position = phase * (float)snapshot[REV].count;
        }

        for (int i = 0; i < SOUND_COUNT; i++) {
            if (!voiceActive[i]) continue;
            VoiceState& v = snapshot[i];

            float rawSample = readInterpolatedHermite4p(v.samples, v.count, v.position, v.loop, v.loopBegin, v.loopEnd);
            mixAccum += rawSample * voiceGains[i];

            v.step = voiceSteps[i];
            bool wrapped = advanceVoice(v);

            // Cycle-quantized jake brake deactivation at sample loop completion
            if (i == JAKE_BRAKE && wrapped && !jakeBrakeRequest) {
                jakeBrakeActive = false;
                engineMuted = false;
                v.active = false;
                voiceActive[JAKE_BRAKE] = false;
            }
        }

        // Start sound processing
        if (currentState == EngineSim::STARTING) {
            if (sounds.slots[START].samples && sounds.slots[START].sampleCount > 0) {
                if (currentStartPos < sounds.slots[START].sampleCount) {
                    float startSample = (float)sounds.slots[START].samples[currentStartPos];
                    mixAccum += startSample * startGain;
                    currentStartPos++;
                }
                if (currentStartPos >= sounds.slots[START].sampleCount) {
                    currentState = EngineSim::RUNNING;
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
                currentState = EngineSim::RUNNING;
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
    engineState = currentState;
    portEXIT_CRITICAL(&voiceMutex);
}

uint8_t SoundSynth::getNextSample() {
    int16_t mono[2] = {0, 0};
    renderBlock(mono, 1);
    int32_t u8 = (mono[0] >> 8) + 128;
    return (uint8_t)constrain(u8, 0, 255);
}
