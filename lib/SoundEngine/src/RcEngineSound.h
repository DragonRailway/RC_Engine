#ifndef RC_ENGINE_SOUND_H
#define RC_ENGINE_SOUND_H

#include <Arduino.h>
#include "SoundTypes.h"

/**
 * @brief Core engine for RC sound and state simulation.
 * 
 * Features:
 * - Fractional step interpolation for pitch shifting (voices speed up with RPM)
 * - Loop points for sustained effect sounds (horn, siren, reversing)
 * - Cylinder-adaptive diesel knock volume (V8, R6, V2 patterns)
 * - RPM-dependent knock volume scaling
 * - Jake brake auto-trigger with engine slowdown
 * - PARKING_BRAKE state in shutdown sequence
 * - Automatic transmission simulation with torque converter
 * - Manual transmission shifting trigger
 * - Voice mixing weights (engine vs effects)
 * - Crawler mode (instant RPM at low volume)
 * - Supercharger start point
 * - Uncoupling separate sound
 * - Sound1 generic channel
 * - ESC ramp time per gear
 * - Tire squeal (speed-dependent)
 * - Hydraulic pump + flow (excavator support)
 * - Track rattle (interval-based)
 * - Bucket rattle (one-shot)
 */
class RcEngineSound {
public:
    enum EngineState {
        OFF,
        STARTING,
        RUNNING,
        STOPPING,
        PARKING_BRAKE
    };

    enum KnockPattern {
        KNOCK_V8,       // Ford/Scania V8: loud at positions 4, 8 of 8
        KNOCK_V8_468,   // Chevy 468: loud at 1, 5, 9, 13 of 16
        KNOCK_R6,       // Inline 6: loud at position 6 of 6
        KNOCK_R6_2,     // Inline 6 alt: loud at 3, 6 of 6
        KNOCK_V2,       // Harley V2: loud at 1, 2 of 4
        KNOCK_UNIFORM   // All pulses equal volume
    };

    enum TransmissionType {
        TRANS_NONE,
        TRANS_MANUAL,
        TRANS_AUTOMATIC
    };

    // Canonical vehicle type (single source of truth — the duplicate in
    // SoundTypes.h was removed). TRUCK and LOCOMOTIVE are the two control
    // surfaces; EXCAVATOR is a recognized stub (control surface deferred);
    // UNKNOWN covers unrecognized config strings with a truck fallback.
    enum VehicleType {
        VEHICLE_TRUCK,
        VEHICLE_LOCOMOTIVE,
        VEHICLE_EXCAVATOR,
        VEHICLE_UNKNOWN
    };

    struct Config {
        char name[32] = "Unknown";
        char description[128] = "";
        char soundSet[32] = "default";
        VehicleType type = VEHICLE_TRUCK;

        struct Engine {
            bool hasEngine = false;
            uint8_t acc = 2;
            uint8_t dec = 2;
            uint8_t brakeDec = 10;
            uint8_t inertia = 10;
            uint16_t escRampTime = 20;
            uint16_t maxRpm = 500;
            uint16_t minRpm = 0;
            float maxPitchFactor = 3.3f;
            uint16_t revSwitchPoint = 50;
            uint16_t idleEndPoint = 300;
            KnockPattern knockPattern = KNOCK_V8;
            uint8_t knockInterval = 8;
            uint8_t knockAdaptiveVolume = 18;
            uint8_t minKnockVolume = 80;
            uint8_t knockStartRpm = 10;
            uint8_t jakeBrakeMinRpm = 60;
            uint8_t jakeBrakeDecelRate = 5;
            uint8_t superchargerStartPoint = 10;
            uint16_t stopDuration = 1400;
        } engine;

        struct Sound {
            uint8_t master = 100;
            uint8_t start = 100;
            uint8_t idle = 100;
            uint8_t idleMin = 0;          // Min idle volume (throttle-dependent scaling)
            uint8_t rev = 100;
            uint8_t revMin = 0;           // Min rev volume (throttle-dependent scaling)
            uint8_t fullThrottle = 100;   // Max volume scaling at full throttle
            uint8_t turbo = 0;
            uint8_t turboMin = 0;         // Min turbo volume at idle
            uint8_t knock = 0;
            uint8_t knockMin = 0;         // Min knock volume at idle
            uint8_t wastegate = 0;
            uint8_t wastegateMin = 0;     // Min wastegate volume
            uint8_t horn = 100;
            uint8_t fan = 0;
            uint8_t jakeBrake = 0;
            uint8_t jakeBrakeMin = 0;     // Min jake brake volume at idle
            uint8_t shifting = 0;
            uint8_t brake = 0;
            uint8_t reversing = 0;
            uint8_t siren = 0;
            uint8_t parkingBrake = 0;
            uint8_t supercharger = 0;
            uint8_t superchargerMin = 10; // Min supercharger volume at idle
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
            uint8_t crawlerModeThreshold = 44;
        } sound;

        struct Transmission {
            TransmissionType type = TRANS_NONE;
            uint8_t numberOfGears = 3;
            uint8_t gearRampTimes[6] = {20, 50, 75, 75, 75, 75};
            uint8_t torqueConverterSlip = 100;
        } transmission;

        struct Features {
            bool hydraulicEnabled = false;
            bool hydrostaticMode = false;
            bool trackRattleEnabled = false;
            bool dumpBedEnabled = false;
            uint8_t tireSquealThreshold = 70;
            uint8_t tireSquealMaxSpeed = 30;
            uint16_t trackRattleIntervalMin = 90;
            uint16_t trackRattleIntervalMax = 500;
        } features;

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

    RcEngineSound();
    virtual ~RcEngineSound();

    void begin(const SoundData& soundData, const Config& config);
    void begin(const SoundData& soundData);
    void setConfig(const Config& config);
    void applyVoiceVolumes();

    void update(int16_t throttle);

    void startEngine();
    void stopEngine();
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

    uint8_t getNextSample();
    void renderBlock(int16_t* interleavedStereoBuffer, size_t frames);

    // Helper: read sample with 4-point Hermite (Catmull-Rom) cubic spline interpolation
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
            // Looping within region [loopBegin, loopEnd]
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
            // Full buffer loop
            int32_t c = (int32_t)count;
            int32_t p = (pos % c + c) % c;
            i1 = p;
            i0 = (p - 1 + c) % c;
            i2 = (p + 1) % c;
            i3 = (p + 2) % c;
        } else {
            // One-shot: clamp to boundaries
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

        // 4-point, 3rd-order Catmull-Rom / Hermite Spline in Horner form
        float c0 = s1;
        float c1 = 0.5f * (s2 - s0);
        float c2 = s0 - 2.5f * s1 + 2.0f * s2 - 0.5f * s3;
        float c3 = 0.5f * (s3 - s0) + 1.5f * (s1 - s2);

        return ((c3 * alpha + c2) * alpha + c1) * alpha + c0;
    }

    // Rational soft-knee saturator / limiter:
    // Transforms accumulated float amplitude [-inf, +inf] into 16-bit PCM [-32767, 32767].
    // Linear (bit-exact) below 2/3 full scale (~ -3.5 dBFS), smoothly compressing asymptotically to 1.0.
    static inline int16_t saturateSoftKnee(float mixAccum) {
        float x = mixAccum / 32767.0f;
        float ax = fabsf(x);
        constexpr float T = 2.0f / 3.0f;       // Linear threshold (~0.6667)
        constexpr float invRange = 1.0f / (1.0f - T); // 3.0
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

    // Legacy/fallback wrapper: read sample with 4-point Hermite interpolation
    static inline int8_t readInterpolated(const int8_t* samples, uint32_t count, float position) {
        return (int8_t)readInterpolatedHermite4p(samples, count, position);
    }

    EngineState getState() const { return state; }
    uint16_t getRpm() const { return currentRpmFixed; }
    uint8_t getGear() const { return selectedGear; }
    float getPitchFactor() const { return pitchFactor; }

    struct VoiceDebugInfo {
        uint8_t id;
        bool active;
        float position;
        uint32_t count;
        uint8_t volume;
        uint32_t loopBegin;
        uint32_t loopEnd;
    };
    void getVoiceDebugSnapshot(VoiceDebugInfo outInfo[SOUND_COUNT]) {
        portENTER_CRITICAL(&voiceMutex);
        for (int i = 0; i < SOUND_COUNT; i++) {
            outInfo[i].id = i;
            outInfo[i].active = voices[i].active;
            outInfo[i].position = voices[i].position;
            outInfo[i].count = voices[i].count;
            outInfo[i].volume = voices[i].volume;
            outInfo[i].loopBegin = voices[i].loopBegin;
            outInfo[i].loopEnd = voices[i].loopEnd;
        }
        portEXIT_CRITICAL(&voiceMutex);
    }

private:
    // Per-voice state for fractional step interpolation
    struct VoiceState {
        float position = 0.0f;
        float step = 1.0f;
        int8_t* samples = nullptr;
        uint32_t count = 0;
        uint8_t volume = 100;
        bool active = false;
        bool pitchShifted = false; // true = engine voice, false = effect voice
        bool oneShot = false;      // true = plays once then deactivates
        bool loop = true;          // true = loops when position >= count
        uint32_t loopBegin = 0;    // Loop region start (0 = use full sample)
        uint32_t loopEnd = 0;      // Loop region end (0 = use full sample)
    };

    // Engine state
    volatile EngineState state;
    SoundData sounds;
    Config cfg;

    // RPM
    float currentRpm = 0.0f;
    uint16_t currentRpmFixed = 0;
    float pitchFactor = 1.0f;

    // Voice array
    VoiceState voices[SOUND_COUNT];

    // Start sound position (special: uses separate sample array)
    volatile uint32_t startPos = 0;

    // Knock trigger state
    uint32_t lastKnockTriggerSample = 0;
    uint8_t curKnockCylinder = 0;

    // Jake brake state
    bool jakeBrakeActive = false;
    bool engineMuted = false;

    // Wastegate state
    bool wastegateTriggered = false;
    uint32_t wastegateTriggerMillis = 0;

    // Transmission state
    uint8_t selectedGear = 1;
    int32_t virtualSpeed = 0;
    uint8_t lastGear = 1; // For manual trans shifting trigger

    // Stop request
    bool engineStopRequested = false;
    int16_t lastThrottle = 0;

    // Timing & Stop State
    uint32_t lastUpdateTime = 0;
    uint32_t stopStartMillis = 0;
    uint16_t stopDurationMs = 1400;
    uint8_t stopVolume = 100;
    float stopPitchFactor = 1.0f;

    // Crawler mode
    bool crawlerMode = false;

    // Track rattle timing
    uint32_t lastTrackRattleTime = 0;

    // Thread safety: mutex for ISR/main loop voice state access
    portMUX_TYPE voiceMutex = portMUX_INITIALIZER_UNLOCKED;

    // Helper: advance voice position with loop region support
    static inline void advanceVoice(VoiceState& v) {
        v.position += v.step;
        if (v.position >= (float)v.count) {
            if (v.loop && v.loopEnd > 0) {
                // Loop within defined region
                float regionLen = (float)(v.loopEnd - v.loopBegin);
                if (regionLen > 0) {
                    v.position = v.loopBegin + fmodf(v.position - v.loopBegin, regionLen);
                    // Clamp: fmodf precision can land past loopEnd
                    if (v.position >= (float)v.loopEnd) v.position = v.loopBegin;
                } else {
                    v.position = v.loopBegin;
                }
            } else if (v.loop) {
                // Full sample loop (backward compatible)
                v.position -= (float)v.count;
            } else {
                // One-shot: deactivate at end
                v.position = 0;
                v.active = false;
            }
        }
    }
};

#endif // RC_ENGINE_SOUND_H
