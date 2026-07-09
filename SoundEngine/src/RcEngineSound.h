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

    struct Config {
        struct Engine {
            uint8_t acc = 2;
            uint8_t dec = 2;
            uint8_t inertia = 10;
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
    void setConfig(const Config& config) { cfg = config; }

    void update(int16_t throttle);

    void startEngine();
    void stopEngine();
    void triggerHorn(bool active);
    void triggerSiren(bool active);
    void triggerBrake(bool active);
    void triggerParkingBrake(bool active);
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

    EngineState getState() const { return state; }
    uint16_t getRpm() const { return currentRpmFixed; }
    uint8_t getGear() const { return selectedGear; }
    float getPitchFactor() const { return pitchFactor; }

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
    EngineState state;
    SoundData sounds;
    Config cfg;

    // RPM
    int32_t currentRpm;
    uint16_t currentRpmFixed;
    float pitchFactor;

    // Voice array
    VoiceState voices[SOUND_COUNT];

    // Start sound position (special: uses separate sample array)
    uint32_t startPos = 0;

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

    // Timing
    uint32_t lastUpdateTime;
    uint32_t attenuatorMillis;
    uint8_t attenuator;
    float stopPitchFactor; // Pitch factor when stopping started

    // Crawler mode
    bool crawlerMode = false;

    // Track rattle timing
    uint32_t lastTrackRattleTime = 0;

    // Thread safety: mutex for ISR/main loop voice state access
    portMUX_TYPE voiceMutex = portMUX_INITIALIZER_UNLOCKED;

    // Helper: read sample with linear interpolation at fractional position
    static inline int8_t readInterpolated(const int8_t* samples, uint32_t count, float position) {
        if (!samples || count == 0) return 0;
        uint32_t pos = (uint32_t)position;
        float frac = position - pos;
        if (pos >= count) pos = count - 1;
        uint32_t next = (pos + 1 < count) ? pos + 1 : 0;
        int32_t s0 = samples[pos];
        int32_t s1 = samples[next];
        return (int8_t)(s0 + (int32_t)((s1 - s0) * frac));
    }

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
