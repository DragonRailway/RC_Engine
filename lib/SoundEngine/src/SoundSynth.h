#ifndef SOUND_SYNTH_H
#define SOUND_SYNTH_H

#include <Arduino.h>
#include "SoundTypes.h"
#include "EngineSim.h"

/**
 * @brief Real-time 32-voice Audio Synthesizer and DSP Mixer.
 * 
 * Features:
 * - 4-point Hermite (Catmull-Rom) cubic spline fractional-step interpolation
 * - Rational soft-knee saturator / limiter preventing 16-bit integer clipping
 * - Dynamic diesel knock pulse synthesis (V8, Chevy 468, R6, Harley V2)
 * - Volume envelopes for idle/rev crossfading, turbo boost, and supercharger
 * - Lockless Core 0 to Core 1 state synchronization via EngineSim
 * - Non-blocking DMA audio block rendering (renderBlock)
 */
class SoundSynth {
public:
    enum KnockPattern {
        KNOCK_V8,       // Ford/Scania V8: loud at positions 4, 8 of 8
        KNOCK_V8_468,   // Chevy 468: loud at 1, 5, 9, 13 of 16
        KNOCK_R6,       // Inline 6: loud at position 6 of 6
        KNOCK_R6_2,     // Inline 6 alt: loud at 3, 6 of 6
        KNOCK_V2,       // Harley V2: loud at 1, 2 of 4
        KNOCK_UNIFORM   // All pulses equal volume
    };

    struct Config {
        struct Sound {
            uint8_t master = 100;
            uint8_t start = 100;
            uint8_t idle = 100;
            uint8_t engineIdle = 100;
            uint8_t idleMin = 0;
            uint8_t rev = 100;
            uint8_t engineRev = 100;
            uint8_t revMin = 0;
            uint8_t fullThrottle = 100;
            uint8_t turbo = 0;
            uint8_t turboMin = 0;
            uint8_t knock = 0;
            uint8_t knockMin = 0;
            uint8_t wastegate = 0;
            uint8_t wastegateMin = 0;
            uint8_t horn = 100;
            uint8_t fan = 0;
            uint8_t jakeBrake = 0;
            uint8_t jakeBrakeMin = 0;
            uint8_t shifting = 0;
            uint8_t brake = 0;
            uint8_t reversing = 0;
            uint8_t siren = 0;
            uint8_t parkingBrake = 0;
            uint8_t supercharger = 0;
            uint8_t superchargerMin = 10;
            uint8_t indicator = 0;
            uint8_t coupling = 0;
            uint8_t uncoupling = 0;
            uint8_t sound1 = 100;
            uint8_t tireSqueal = 0;
            uint8_t hydraulicPump = 0;
            uint8_t hydraulicFlow = 0;
            uint8_t trackRattle = 0;
            uint8_t bucketRattle = 0;
            uint8_t bell = 0;
            uint8_t door = 0;
            uint8_t scanner = 0;
            uint8_t music = 0;
            uint8_t whistle = 0;
            uint8_t gun = 0;
            uint8_t outOfFuel = 0;
            uint8_t others = 0;
            uint8_t engineMixWeight = 100;
            uint8_t effectMixWeight = 100;
        } sound;

        struct EngineKnock {
            KnockPattern knockPattern = KNOCK_V8;
            uint8_t knockInterval = 8;
            uint8_t knockAdaptiveVolume = 18;
            uint8_t minKnockVolume = 80;
            uint8_t knockStartRpm = 10;
            uint16_t maxRpm = 500;
            uint16_t revSwitchPoint = 50;
            uint16_t idleEndPoint = 300;
            uint8_t superchargerStartPoint = 10;
        } engine;

        struct LoopPoints {
            uint32_t hornBegin = 0;
            uint32_t hornEnd = 0;
            uint32_t sirenBegin = 0;
            uint32_t sirenEnd = 0;
            uint32_t reversingBegin = 0;
            uint32_t reversingEnd = 0;
            uint32_t sound1Begin = 0;
            uint32_t sound1End = 0;
        } loopPoints;
    };

    SoundSynth();
    virtual ~SoundSynth();

    void begin(const SoundData& soundData, const Config& config);
    void begin(const SoundData& soundData);
    void setConfig(const Config& config);
    void applyVoiceVolumes();

    // ── Synchronize state from EngineSim (50 Hz loop on Core 0) ──
    void syncState(const EngineSim& sim);

    // ── Effect Triggers ──
    void triggerHorn(bool active);
    void triggerSiren(bool active);
    void triggerBrake(bool active);
    void triggerParkingBrake(bool active);
    void triggerJakeBrake(bool active);
    void triggerWastegate(bool active);
    void triggerReversing(bool active);
    void triggerShifting(bool active);
    void triggerIndicator(bool active);
    void triggerCoupling(bool active);
    void triggerUncoupling(bool active);
    void triggerSound1(bool active);
    void triggerTireSqueal(bool active);
    void triggerHydraulicPump(bool active);
    void triggerHydraulicFlow(bool active);
    void triggerTrackRattle(bool active);
    void triggerBucketRattle(bool active);
    void triggerDumpBed(bool active);
    void triggerBell(bool active);
    void triggerDoor(bool active);
    void triggerScanner(bool active);
    void triggerMusic(bool active);
    void triggerWhistle(bool active);
    void triggerGun(bool active);
    void triggerOutOfFuel(bool active);
    void triggerOthers(bool active);

    // ── DSP Block Renderer (22.05 kHz audioTask on Core 1) ──
    void renderBlock(int16_t* interleavedStereoBuffer, size_t frames);
    uint8_t getNextSample();
    EngineSim::State getState() const { return engineState; }
    bool isStartSoundComplete() const { return (engineState != EngineSim::STARTING); }

    // ── Hermite 4-point Interpolator ──
    static inline float readInterpolatedHermite4p(const int8_t* samples, uint32_t count, float position,
                                                  bool loop = true, uint32_t loopBegin = 0, uint32_t loopEnd = 0) {
        if (!samples || count == 0) return 0.0f;
        if (count == 1) return (float)samples[0];

        int32_t pos = (int32_t)position;
        float alpha = position - (float)pos;
        if (alpha < 0.0f) alpha = 0.0f;
        else if (alpha > 1.0f) alpha = 1.0f;

        int32_t i0, i1, i2, i3;
        if (loop && loopEnd > loopBegin && loopEnd <= count) {
            int32_t lStart = (int32_t)loopBegin;
            int32_t lLen = (int32_t)(loopEnd - loopBegin);
            if (pos < lStart) {
                i1 = (pos >= 0) ? pos : 0;
                i0 = (i1 > 0) ? i1 - 1 : 0;
                i2 = (i1 + 1 < (int32_t)count) ? i1 + 1 : i1;
                i3 = (i1 + 2 < (int32_t)count) ? i1 + 2 : i2;
            } else {
                int32_t offset = (pos - lStart) % lLen;
                if (offset < 0) offset += lLen;
                i1 = lStart + offset;
                i0 = lStart + ((offset - 1 + lLen) % lLen);
                i2 = lStart + ((offset + 1) % lLen);
                i3 = lStart + ((offset + 2) % lLen);
            }
        } else if (loop) {
            int32_t c = (int32_t)count;
            int32_t p = (pos % c + c) % c;
            i1 = p;
            i0 = (p - 1 + c) % c;
            i2 = (p + 1) % c;
            i3 = (p + 2) % c;
        } else {
            int32_t maxIdx = (int32_t)count - 1;
            i1 = (pos < 0) ? 0 : (pos > maxIdx ? maxIdx : pos);
            i0 = (i1 > 0) ? i1 - 1 : 0;
            i2 = (i1 + 1 <= maxIdx) ? i1 + 1 : maxIdx;
            i3 = (i1 + 2 <= maxIdx) ? i1 + 2 : maxIdx;
        }

        float s0 = (float)samples[i0];
        float s1 = (float)samples[i1];
        float s2 = (float)samples[i2];
        float s3 = (float)samples[i3];

        float c0 = s1;
        float c1 = 0.5f * (s2 - s0);
        float c2 = s0 - 2.5f * s1 + 2.0f * s2 - 0.5f * s3;
        float c3 = 0.5f * (s3 - s0) + 1.5f * (s1 - s2);

        return ((c3 * alpha + c2) * alpha + c1) * alpha + c0;
    }

    // ── Rational Soft-Knee Saturator ──
    static inline int16_t saturateSoftKnee(float mixAccum) {
        float x = mixAccum / 32767.0f;
        float ax = fabsf(x);
        constexpr float T = 2.0f / 3.0f;
        constexpr float invRange = 1.0f / (1.0f - T);
        float y;
        if (ax <= T) {
            y = x;
        } else {
            float delta = ax - T;
            float sat = T + delta / (1.0f + delta * invRange);
            y = (x < 0.0f) ? -sat : sat;
        }
        int32_t outSample = (int32_t)roundf(y * 32767.0f);
        return (int16_t)constrain(outSample, -32768, 32767);
    }

private:
    struct VoiceState {
        float position = 0.0f;
        float step = 1.0f;
        int8_t* samples = nullptr;
        uint32_t count = 0;
        uint8_t volume = 100;
        bool active = false;
        bool pitchShifted = false;
        bool oneShot = false;
        bool loop = true;
        uint32_t loopBegin = 0;
        uint32_t loopEnd = 0;
    };

    SoundData sounds;
    Config cfg;
    VoiceState voices[SOUND_COUNT];

    // Engine simulation snapshot
    volatile EngineSim::State engineState = EngineSim::OFF;
    volatile uint16_t currentRpm = 0;
    volatile float pitchFactor = 1.0f;
    volatile float currentThrottleFaded = 0.0f;
    volatile bool jakeBrakeActive = false;
    volatile bool jakeBrakeRequest = false;
    volatile bool engineMuted = false;

    // Start sound position
    volatile uint32_t startPos = 0;

    // Knock synthesis
    uint32_t lastKnockTriggerSample = 0;
    uint8_t curKnockCylinder = 0;

    portMUX_TYPE voiceMutex = portMUX_INITIALIZER_UNLOCKED;

    static inline bool advanceVoice(VoiceState& v) {
        v.position += v.step;
        bool wrapped = false;
        if (v.position >= (float)v.count) {
            wrapped = true;
            if (v.loop && v.loopEnd > 0) {
                float regionLen = (float)(v.loopEnd - v.loopBegin);
                if (regionLen > 0) {
                    v.position = v.loopBegin + fmodf(v.position - v.loopBegin, regionLen);
                    if (v.position >= (float)v.loopEnd) v.position = v.loopBegin;
                } else {
                    v.position = v.loopBegin;
                }
            } else if (v.loop) {
                v.position -= (float)v.count;
            } else {
                v.position = 0;
                v.active = false;
            }
        }
        return wrapped;
    }
};

#endif // SOUND_SYNTH_H
