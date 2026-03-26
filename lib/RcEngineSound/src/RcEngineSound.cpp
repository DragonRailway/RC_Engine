#include "RcEngineSound.h"

RcEngineSound::RcEngineSound() :
    state(OFF),
    currentRpm(0),
    currentRpmFixed(0),
    curEngineSample(0),
    curRevSample(0),
    curStartSample(0),
    curHornSample(0),
    hornActive(false),
    engineStopRequested(false),
    lastUpdateTime(0),
    attenuatorMillis(0),
    attenuator(1)
{}

void RcEngineSound::begin(const SoundData& soundData, const Config& config) {
    sounds = soundData;
    cfg = config;
}

void RcEngineSound::begin(const SoundData& soundData) {
    begin(soundData, Config());
}

void RcEngineSound::startEngine() {
    if (state == OFF) {
        state = STARTING;
        curStartSample = 0;
    }
}

void RcEngineSound::stopEngine() {
    if (state == RUNNING) {
        engineStopRequested = true;
    }
}

void RcEngineSound::triggerHorn(bool active) {
    hornActive = active;
}

void RcEngineSound::update(int16_t throttle) {
    uint32_t now = millis();
    if (lastUpdateTime == 0) lastUpdateTime = now;
    lastUpdateTime = now;

    int32_t targetRpm = abs(throttle);
    if (targetRpm > cfg.maxRpm) targetRpm = cfg.maxRpm;

    if (state == RUNNING) {
        if (targetRpm > currentRpm + cfg.acc) {
            currentRpm += cfg.acc;
        } else if (targetRpm < currentRpm - cfg.dec) {
            currentRpm -= cfg.dec;
        } else {
            currentRpm = targetRpm;
        }

        if (engineStopRequested && currentRpm < 50) {
            state = STOPPING;
            engineStopRequested = false;
            attenuator = 1;
            attenuatorMillis = now;
        }
    } else if (state == OFF) {
        currentRpm = 0;
    }

    currentRpmFixed = currentRpm;
}

uint8_t RcEngineSound::getNextSample() {
    int32_t mixed = 0;
    int32_t engineSample = 0;
    int32_t hornSample = 0;

    switch (state) {
        case STARTING:
            if (curStartSample < sounds.startSampleCount) {
                engineSample = (int8_t)sounds.startSamples[curStartSample];
                engineSample = engineSample * cfg.startVolume / 100;
                curStartSample++;
            } else {
                state = RUNNING;
                curEngineSample = 0;
                curRevSample = 0;
            }
            break;

        case RUNNING: {
            int32_t idleS = 0;
            int32_t revS = 0;

            // Idle sound
            if (curEngineSample < sounds.sampleCount) {
                idleS = (int8_t)sounds.samples[curEngineSample];
                curEngineSample++;
            } else {
                curEngineSample = 0;
            }

            // Rev sound (optional cross-fade)
            if (sounds.revSamples && sounds.revSampleCount > 0) {
                if (curRevSample < sounds.revSampleCount) {
                    revS = (int8_t)sounds.revSamples[curRevSample];
                    curRevSample++;
                } else {
                    curRevSample = 0;
                }

                // Mixing logic similar to src.ino
                int16_t idleProportion = 100;
                if (currentRpmFixed > cfg.revSwitchPoint) {
                    idleProportion = map(currentRpmFixed, cfg.idleEndPoint, cfg.revSwitchPoint, 0, 100);
                    if (idleProportion < 0) idleProportion = 0;
                    if (idleProportion > 100) idleProportion = 100;
                }
                
                idleS = (idleS * cfg.idleVolume / 100) * idleProportion / 100;
                revS = (revS * cfg.revVolume / 100) * (100 - idleProportion) / 100;
                engineSample = idleS + revS;
            } else {
                engineSample = idleS * cfg.idleVolume / 100;
            }
            break;
        }

        case STOPPING:
            if (curEngineSample < sounds.sampleCount) {
                engineSample = (int8_t)sounds.samples[curEngineSample] * cfg.idleVolume / 100 / attenuator;
                curEngineSample++;
            } else {
                curEngineSample = 0;
            }

            if (millis() - attenuatorMillis > 100) {
                attenuatorMillis = millis();
                attenuator++;
            }

            if (attenuator >= 50) {
                state = OFF;
            }
            break;

        default:
            engineSample = 0;
            break;
    }

    // Horn mixing
    if (hornActive && sounds.hornSamples) {
        if (curHornSample < sounds.hornSampleCount) {
            hornSample = (int8_t)sounds.hornSamples[curHornSample] * cfg.hornVolume / 100;
            curHornSample++;
        } else {
            curHornSample = 0;
        }
    }

    // Mix and apply master volume
    mixed = engineSample + (hornSample / 2);
    mixed = (mixed * cfg.masterVolume / 100) + 128; // Add DC offset last

    return (uint8_t)constrain(mixed, 0, 255);
}
