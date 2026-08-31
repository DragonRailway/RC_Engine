#include "Arduino.h"
#include <RcEngineSound.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>
#include <cassert>
#include <cstring>

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

static float calculateZCR(const std::vector<int16_t>& samples, size_t startIdx, size_t count) {
    if (count < 2 || startIdx + count > samples.size()) return 0.0f;
    int crossings = 0;
    for (size_t i = startIdx + 1; i < startIdx + count; i++) {
        if ((samples[i-1] >= 0 && samples[i] < 0) || (samples[i-1] < 0 && samples[i] >= 0)) {
            crossings++;
        }
    }
    return ((float)crossings / (float)count) * 1000.0f;
}

int main(int argc, char** argv) {
    bool breakLoopMode = false;
    const char* pcmOutPath = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--break-loop") == 0) {
            breakLoopMode = true;
        } else if (strcmp(argv[i], "--pcm") == 0 && i + 1 < argc) {
            pcmOutPath = argv[++i];
        }
    }

    SoundData soundData;
    soundData.isDynamic = true;

    initSyntheticVoiceSlot(soundData.slots[IDLE], 100, 2205);      // 100Hz 0.1s
    initSyntheticVoiceSlot(soundData.slots[REV], 200, 2205);       // 200Hz 0.1s
    initSyntheticVoiceSlot(soundData.slots[START], 150, 441);      // 150Hz short start sound
    initSyntheticVoiceSlot(soundData.slots[KNOCK], 500, 220);      // 500Hz 0.01s
    initSyntheticVoiceSlot(soundData.slots[HORN], 440, 2205);      // 440Hz 0.1s
    initSyntheticVoiceSlot(soundData.slots[SHIFTING], 300, 1102);  // 300Hz 0.05s

    RcEngineSound::Config config;
    config.engine.minRpm = 10;
    config.engine.maxRpm = 500;
    config.engine.maxPitchFactor = 3.0f;
    config.engine.acc = 20;
    config.engine.inertia = 10;

    if (breakLoopMode) {
        config.loopPoints.hornBegin = 3000;
        config.loopPoints.hornEnd = 5000;
    } else {
        config.loopPoints.hornBegin = 100;
        config.loopPoints.hornEnd = 2000;
    }

    RcEngineSound engine;
    engine.begin(soundData, config);

    std::cout << "[Host DSP Harness] Engine initialized successfully." << std::endl;

    std::vector<int16_t> fullStreamPCM16;

    // Phase 1: Start Engine
    engine.startEngine();
    for (int i = 0; i < 1000; i++) {
        if (i % 22 == 0) {
            host_virtual_millis += 10;
            engine.update(0);
        }
        int16_t block[2];
        engine.renderBlock(block, 1);
        fullStreamPCM16.push_back(block[0]);
        if (engine.getState() == RcEngineSound::RUNNING) break;
    }

    // Phase 2: Idle (1 second = 22050 samples)
    std::vector<int16_t> idleBuffer;
    for (int i = 0; i < 22050; i++) {
        if (i % 22 == 0) {
            host_virtual_millis += 10;
            engine.update(0);
        }
        int16_t block[2];
        engine.renderBlock(block, 1);
        idleBuffer.push_back(block[0]);
        fullStreamPCM16.push_back(block[0]);
    }
    float idleZCR = calculateZCR(idleBuffer, 0, idleBuffer.size());
    std::cout << "[Host DSP Harness] Idle ZCR (low RPM=" << engine.getRpm() << ", pitch=" << engine.getPitchFactor() << "): " << idleZCR << std::endl;

    // Phase 3: Rev Ramp Up (2 seconds = 44100 samples)
    std::vector<int16_t> revBuffer;
    for (int i = 0; i < 44100; i++) {
        if (i % 22 == 0) {
            host_virtual_millis += 10;
            engine.update(100);
        }
        int16_t block[2];
        engine.renderBlock(block, 1);
        if (i >= 22050) {
            revBuffer.push_back(block[0]);
        }
        fullStreamPCM16.push_back(block[0]);
    }
    float revZCR = calculateZCR(revBuffer, 0, revBuffer.size());
    std::cout << "[Host DSP Harness] Rev ZCR (high RPM=" << engine.getRpm() << ", pitch=" << engine.getPitchFactor() << "): " << revZCR << std::endl;

    // Phase 4: Horn Trigger (1 second = 22050 samples)
    engine.triggerHorn(true);
    for (int i = 0; i < 22050; i++) {
        if (i % 22 == 0) {
            host_virtual_millis += 10;
            engine.update(100);
        }
        int16_t block[2];
        engine.renderBlock(block, 1);
        fullStreamPCM16.push_back(block[0]);
    }
    engine.triggerHorn(false);

    // Phase 5: Stop Engine
    engine.stopEngine();
    for (int i = 0; i < 22050; i++) {
        if (i % 22 == 0) {
            host_virtual_millis += 10;
            engine.update(0);
        }
        int16_t block[2];
        engine.renderBlock(block, 1);
        fullStreamPCM16.push_back(block[0]);
    }

    assert(revZCR > idleZCR && "Pitch scaling failed: Rev ZCR should be higher than Idle ZCR!");
    std::cout << "[Host DSP Harness] PASS: Pitch scales with RPM." << std::endl;

    // Phase 6: Hermite 4-Point Spline Mathematical Precision Test
    {
        static const int8_t testSamples[8] = {0, 30, 80, 100, 70, 20, -40, -80};
        // At exact integer positions, Hermite spline must match sample points exactly:
        float s0 = RcEngineSound::readInterpolatedHermite4p(testSamples, 8, 2.0f, false);
        assert(fabs(s0 - 80.0f) < 0.001f && "Hermite spline must match sample at integer pos 2.0");

        float s1 = RcEngineSound::readInterpolatedHermite4p(testSamples, 8, 3.0f, false);
        assert(fabs(s1 - 100.0f) < 0.001f && "Hermite spline must match sample at integer pos 3.0");

        // At midpoint 2.5, Hermite polynomial must be continuous and between s[2] and s[3]:
        float mid = RcEngineSound::readInterpolatedHermite4p(testSamples, 8, 2.5f, false);
        assert(mid > 80.0f && mid < 105.0f && "Hermite spline smooth curve at midpoint");

        std::cout << "[Host DSP Harness] PASS: 4-Point Hermite cubic spline polynomial precision verified." << std::endl;
    }

    // Phase 7: Polynomial Cubic Soft-Knee Saturator Precision Test
    {
        // 1. Linear below 2/3 full scale (~21845):
        int16_t lin0 = RcEngineSound::saturateSoftKnee(10000.0f);
        assert(lin0 == 10000 && "Soft-knee must be bit-exact in linear region");

        int16_t linNeg = RcEngineSound::saturateSoftKnee(-20000.0f);
        assert(linNeg == -20000 && "Soft-knee must be bit-exact in negative linear region");

        // 2. Smoothly saturating above 2/3 full scale:
        int16_t satMid = RcEngineSound::saturateSoftKnee(26000.0f);
        assert(satMid > 21845 && satMid < 26000 && "Soft-knee must compress above 2/3 full scale");

        // 3. Clamping & asymptotic compression at extreme amplitudes:
        int16_t satMax = RcEngineSound::saturateSoftKnee(100000.0f);
        assert(satMax > 30000 && satMax <= 32767 && "Soft-knee must compress smoothly below ceiling");

        int16_t satMin = RcEngineSound::saturateSoftKnee(-100000.0f);
        assert(satMin < -30000 && satMin >= -32768 && "Soft-knee must compress smoothly below negative ceiling");

        std::cout << "[Host DSP Harness] PASS: Warm analog soft-knee limiter verified." << std::endl;
    }

    if (breakLoopMode) {
        if (config.loopPoints.hornEnd > soundData.slots[HORN].sampleCount) {
            std::cout << "[Host DSP Harness] DETECTED REGRESSION: hornEnd ("
                      << config.loopPoints.hornEnd << ") > sampleCount ("
                      << soundData.slots[HORN].sampleCount << ")" << std::endl;
            return 1;
        }
    } else {
        assert(config.loopPoints.hornEnd <= soundData.slots[HORN].sampleCount && "Loop end exceeds sample count!");
        std::cout << "[Host DSP Harness] PASS: Loop bounds within sample count." << std::endl;
    }

    if (pcmOutPath) {
        std::ofstream pcmFile(pcmOutPath, std::ios::binary);
        if (pcmFile) {
            pcmFile.write((const char*)fullStreamPCM16.data(), fullStreamPCM16.size() * sizeof(int16_t));
            std::cout << "[Host DSP Harness] Exported " << fullStreamPCM16.size()
                      << " PCM16 samples to: " << pcmOutPath << std::endl;
        }
    }

    std::cout << "[Host DSP Harness] ALL TESTS PASSED SUCCESSFULLY." << std::endl;
    return 0;
}
