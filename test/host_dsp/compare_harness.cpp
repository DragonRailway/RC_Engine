#include "Arduino.h"
#include <RcEngineSound.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>
#include <cassert>
#include <cstring>
#include <string>

uint32_t host_virtual_millis = 0;
DummySerial Serial;

static void initSyntheticVoiceSlot(SoundSlot& slot, int frequency, int lengthSamples) {
    slot.sampleCount = lengthSamples;
    slot.samples = (int8_t*)malloc(lengthSamples);
    for (int i = 0; i < lengthSamples; i++) {
        float t = (float)i / 22050.0f;
        slot.samples[i] = (int8_t)(sinf(2.0f * M_PI * (float)frequency * t) * 100.0f);
    }
}

class ReferenceEngineSimulator {
public:
    float currentPos = 0.0f;
    float hornPos = 0.0f;
    float indicatorPos = 0.0f;
    float revBeepPos = 0.0f;

    bool hornActive = false;
    bool indicatorActive = false;
    bool revBeepActive = false;

    int8_t* idleSamples = NULL;
    uint32_t idleCount = 0;

    int8_t* hornSamples = NULL;
    uint32_t hornCount = 0;

    int8_t* indicatorSamples = NULL;
    uint32_t indicatorCount = 0;

    int8_t* revBeepSamples = NULL;
    uint32_t revBeepCount = 0;

    uint8_t masterVolume = 80;

    void begin(SoundData& soundData, uint8_t vol) {
        idleSamples = soundData.slots[IDLE].samples;
        idleCount = soundData.slots[IDLE].sampleCount;

        hornSamples = soundData.slots[HORN].samples;
        hornCount = soundData.slots[HORN].sampleCount;

        indicatorSamples = soundData.slots[INDICATOR].samples;
        indicatorCount = soundData.slots[INDICATOR].sampleCount;

        revBeepSamples = soundData.slots[REVERSING].samples;
        revBeepCount = soundData.slots[REVERSING].sampleCount;

        masterVolume = vol;
        currentPos = 0.0f;
        hornPos = 0.0f;
        indicatorPos = 0.0f;
        revBeepPos = 0.0f;
    }

    void triggerHorn(bool active) {
        if (active && !hornActive) hornPos = 0.0f;
        hornActive = active;
    }

    void triggerIndicator(bool active) {
        if (active && !indicatorActive) indicatorPos = 0.0f;
        indicatorActive = active;
    }

    void triggerReversing(bool active) {
        if (active && !revBeepActive) revBeepPos = 0.0f;
        revBeepActive = active;
    }

    uint8_t renderSample(float pitchFactor) {
        int32_t engineVal = 0;
        int32_t effectVal = 0;

        // Engine Voice (Pitch Shifted, Looped)
        if (idleSamples && idleCount > 0) {
            currentPos += pitchFactor;
            if (currentPos >= (float)idleCount) {
                currentPos = fmodf(currentPos, (float)idleCount);
            }
            engineVal += idleSamples[(uint32_t)currentPos];
        }

        // Horn Voice (Fixed Pitch, Looped while held)
        if (hornActive && hornSamples && hornCount > 0) {
            hornPos += 1.0f;
            if (hornPos >= (float)hornCount) hornPos = fmodf(hornPos, (float)hornCount);
            effectVal += (int32_t)hornSamples[(uint32_t)hornPos] * 80 / 100;
        }

        // Indicator Voice (Fixed Pitch, One-Shot)
        if (indicatorActive && indicatorSamples && indicatorCount > 0) {
            if (indicatorPos < (float)indicatorCount) {
                effectVal += (int32_t)indicatorSamples[(uint32_t)indicatorPos] * 80 / 100;
                indicatorPos += 1.0f;
                if (indicatorPos >= (float)indicatorCount) {
                    indicatorActive = false;
                }
            }
        }

        // Reversing Beep Voice (Fixed Pitch, Looped while active)
        if (revBeepActive && revBeepSamples && revBeepCount > 0) {
            revBeepPos += 1.0f;
            if (revBeepPos >= (float)revBeepCount) revBeepPos = fmodf(revBeepPos, (float)revBeepCount);
            effectVal += (int32_t)revBeepSamples[(uint32_t)revBeepPos] * 80 / 100;
        }

        int32_t engineScaled = engineVal * 80 / 100;
        int32_t effectScaled = effectVal * 50 / 100;
        int32_t mixed = (engineScaled + effectScaled) * masterVolume / 100 + 128;
        return (uint8_t)constrain(mixed, 0, 255);
    }
};

int main(int argc, char** argv) {
    int scenario = 1;
    const char* refPcmPath = "data/ref_scenario.raw";
    const char* rcbPcmPath = "data/rcb_scenario.raw";

    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--scenario" && i + 1 < argc) {
            scenario = std::stoi(argv[++i]);
        } else if (std::string(argv[i]) == "--ref-out" && i + 1 < argc) {
            refPcmPath = argv[++i];
        } else if (std::string(argv[i]) == "--rcb-out" && i + 1 < argc) {
            rcbPcmPath = argv[++i];
        }
    }

    SoundData soundData;
    soundData.isDynamic = true;

    initSyntheticVoiceSlot(soundData.slots[IDLE], 100, 2205);      // 100Hz 0.1s
    initSyntheticVoiceSlot(soundData.slots[REV], 200, 2205);       // 200Hz 0.1s
    initSyntheticVoiceSlot(soundData.slots[START], 150, 441);      // 150Hz
    initSyntheticVoiceSlot(soundData.slots[KNOCK], 500, 220);      // 500Hz
    initSyntheticVoiceSlot(soundData.slots[HORN], 440, 2205);      // 440Hz
    initSyntheticVoiceSlot(soundData.slots[INDICATOR], 800, 441);   // 800Hz
    initSyntheticVoiceSlot(soundData.slots[REVERSING], 1200, 1102); // 1200Hz
    initSyntheticVoiceSlot(soundData.slots[SHIFTING], 300, 1102);  // 300Hz

    ReferenceEngineSimulator refEngine;
    refEngine.begin(soundData, 80);

    RcEngineSound::Config config;
    config.engine.minRpm = 10;
    config.engine.maxRpm = 500;
    config.engine.maxPitchFactor = 3.0f;
    config.engine.acc = 25;
    config.engine.inertia = 10;
    config.sound.idle = 80;
    config.sound.horn = 80;
    config.sound.indicator = 80;
    config.sound.reversing = 80;
    config.sound.engineMixWeight = 80;
    config.sound.effectMixWeight = 50;

    RcEngineSound rcbEngine;
    rcbEngine.begin(soundData, config);
    rcbEngine.startEngine();

    // Drain START sound
    for (int i = 0; i < 1000; i++) {
        if (i % 22 == 0) {
            host_virtual_millis += 10;
            rcbEngine.update(0);
        }
        rcbEngine.getNextSample();
        if (rcbEngine.getState() == RcEngineSound::RUNNING) break;
    }

    std::vector<int16_t> refPCM16;
    std::vector<int16_t> rcbPCM16;

    int totalSamples = 110250; // 5.0 seconds

    for (int i = 0; i < totalSamples; i++) {
        int32_t throttle = 0;
        bool hornOn = false;
        bool indicatorOn = false;
        bool revBeepOn = false;

        switch (scenario) {
            case 1: // Slow Throttle Ramp
                if (i >= 22050 && i < 44100) throttle = 30;
                else if (i >= 44100 && i < 66150) throttle = 70;
                else if (i >= 66150 && i < 88200) throttle = 100;
                break;

            case 2: // Rapid Throttle Pulse & Deceleration
                if ((i >= 22050 && i < 33075) || (i >= 55125 && i < 66150)) throttle = 100;
                break;

            case 3: // Reversing Beep & Gear Shift
                if (i >= 22050 && i < 66150) {
                    revBeepOn = true;
                }
                throttle = 40;
                break;

            case 4: // Turn Indicator & Horn Overlay
                if (i == 22050 || i == 33075 || i == 44100 || i == 55125) indicatorOn = true;
                if (i >= 44100 && i < 66150) hornOn = true;
                break;

            case 5: // Full Stress Mixed Combination
                if (i >= 22050 && i < 88200) {
                    throttle = (i % 44100 < 22050) ? 80 : 20;
                    if (i % 11025 == 0) indicatorOn = true;
                    if (i >= 33075 && i < 55125) hornOn = true;
                    if (i >= 66150) revBeepOn = true;
                }
                break;
        }

        if (i % 22 == 0) {
            host_virtual_millis += 10;
            rcbEngine.update(throttle);
        }

        if (hornOn) {
            refEngine.triggerHorn(true);
            rcbEngine.triggerHorn(true);
        } else {
            refEngine.triggerHorn(false);
            rcbEngine.triggerHorn(false);
        }

        if (indicatorOn) {
            refEngine.triggerIndicator(true);
            rcbEngine.triggerIndicator(true);
        }

        if (revBeepOn) {
            refEngine.triggerReversing(true);
            rcbEngine.triggerReversing(true);
        } else {
            refEngine.triggerReversing(false);
            rcbEngine.triggerReversing(false);
        }

        float currentPitch = rcbEngine.getPitchFactor();

        uint8_t s_ref = refEngine.renderSample(currentPitch);
        uint8_t s_rcb = rcbEngine.getNextSample();

        int16_t pcm_ref = ((int16_t)s_ref - 128) * 256;
        int16_t pcm_rcb = ((int16_t)s_rcb - 128) * 256;

        refPCM16.push_back(pcm_ref);
        rcbPCM16.push_back(pcm_rcb);
    }

    std::ofstream fRef(refPcmPath, std::ios::binary);
    if (fRef) fRef.write((const char*)refPCM16.data(), refPCM16.size() * sizeof(int16_t));

    std::ofstream fRcb(rcbPcmPath, std::ios::binary);
    if (fRcb) fRcb.write((const char*)rcbPCM16.data(), rcbPCM16.size() * sizeof(int16_t));

    std::cout << "[Harness] Scenario " << scenario << " exported to " << refPcmPath << " and " << rcbPcmPath << std::endl;
    return 0;
}
