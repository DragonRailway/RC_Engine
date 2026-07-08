#ifndef RC_ENGINE_SOUND_H
#define RC_ENGINE_SOUND_H

#include <Arduino.h>

/**
 * @brief Structure to hold engine sound samples and metadata.
 */
struct SoundData {
    int8_t* samples = nullptr;
    uint32_t sampleCount = 0;
    uint16_t sampleRate = 22050;
    
    int8_t* startSamples = nullptr;
    uint32_t startSampleCount = 0;
    
    int8_t* revSamples = nullptr;
    uint32_t revSampleCount = 0;
    
    int8_t* turboSamples = nullptr;
    uint32_t turboSampleCount = 0;
    
    int8_t* knockSamples = nullptr;
    uint32_t knockSampleCount = 0;
    
    int8_t* wastegateSamples = nullptr;
    uint32_t wastegateSampleCount = 0;
    
    int8_t* hornSamples = nullptr;
    uint32_t hornSampleCount = 0;
    uint16_t hornSampleRate = 22050;

    int8_t* jakeBrakeSamples = nullptr;
    uint32_t jakeBrakeSampleCount = 0;

    int8_t* fanSamples = nullptr;
    uint32_t fanSampleCount = 0;

    int8_t* sirenSamples = nullptr;
    uint32_t sirenSampleCount = 0;

    int8_t* brakeSamples = nullptr;
    uint32_t brakeSampleCount = 0;

    int8_t* reversingSamples = nullptr;
    uint32_t reversingSampleCount = 0;

    int8_t* parkingBrakeSamples = nullptr;
    uint32_t parkingBrakeSampleCount = 0;

    int8_t* superchargerSamples = nullptr;
    uint32_t superchargerSampleCount = 0;

    int8_t* shiftingSamples = nullptr;
    uint32_t shiftingSampleCount = 0;

    int8_t* indicatorSamples = nullptr;
    uint32_t indicatorSampleCount = 0;

    int8_t* couplingSamples = nullptr;
    uint32_t couplingSampleCount = 0;

    int8_t* uncouplingSamples = nullptr;
    uint32_t uncouplingSampleCount = 0;

    int8_t* sound1Samples = nullptr;
    uint32_t sound1SampleCount = 0;

    bool isDynamic = false; // Set to true if allocated in PSRAM/Heap
};

/**
 * @brief Core engine for RC sound and state simulation.
 * 
 * Features:
 * - Fractional step interpolation for pitch shifting (voices speed up with RPM)
 * - Cylinder-adaptive diesel knock volume (V8, R6, V2 patterns)
 * - Jake brake auto-trigger with engine slowdown
 * - PARKING_BRAKE state in shutdown sequence
 * - Automatic transmission simulation with torque converter
 * - Supercharger start point
 * - Uncoupling separate sound
 * - Sound1 generic channel
 * - ESC ramp time per gear
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
        // Engine behavior
        uint8_t acc = 2;
        uint8_t dec = 2;
        uint8_t inertia = 10; // 0-100, higher = slower RPM response
        uint16_t maxRpm = 500;
        uint16_t minRpm = 0;
        
        // Sound volumes
        uint8_t masterVolume = 100;
        uint8_t startVolume = 100;
        uint8_t idleVolume = 100;
        uint8_t revVolume = 100;
        uint8_t turboVolume = 0;
        uint8_t knockVolume = 0;
        uint8_t wastegateVolume = 0;
        uint8_t hornVolume = 100;
        uint8_t fanVolume = 0;
        uint8_t jakeBrakeVolume = 0;
        uint8_t shiftingVolume = 0;
        uint8_t brakeVolume = 0;
        uint8_t reversingVolume = 0;
        uint8_t sirenVolume = 0;
        uint8_t parkingBrakeVolume = 0;
        uint8_t superchargerVolume = 0;
        uint8_t indicatorVolume = 0;
        uint8_t couplingVolume = 0;
        uint8_t uncouplingVolume = 0;
        uint8_t sound1Volume = 100;

        // Pitch shifting
        float maxPitchFactor = 3.3f; // Max pitch multiplier at full RPM

        // Idle/Rev cross-fade
        uint16_t revSwitchPoint = 50;
        uint16_t idleEndPoint = 300;

        // Diesel knock
        KnockPattern knockPattern = KNOCK_V8;
        uint8_t knockInterval = 8;          // Pulses per idle loop
        uint8_t knockAdaptiveVolume = 18;   // Volume % for secondary pulses

        // Jake brake
        uint8_t jakeBrakeMinRpm = 60;  // Minimum RPM % for jake brake to activate
        uint8_t jakeBrakeDecelRate = 5; // How fast jake brake slows engine

        // Supercharger
        uint8_t superchargerStartPoint = 10; // RPM% where supercharger becomes audible

        // Transmission
        TransmissionType transmissionType = TRANS_NONE;
        uint8_t numberOfGears = 3;
        uint8_t gearRampTimes[6] = {20, 50, 75, 75, 75, 75}; // Per-gear acceleration response

        // Transmission (legacy)
        bool automatic = false;
        uint16_t clutchEngagingPoint = 100;
        uint16_t maxRpmPercentage = 310;

        struct Lights {
            bool xenon = false;
            bool doubleFlashBlue = false;
        } lights;
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

    uint8_t getNextSample();

    EngineState getState() const { return state; }
    uint16_t getRpm() const { return currentRpmFixed; }
    uint8_t getGear() const { return selectedGear; }
    float getPitchFactor() const { return pitchFactor; }

private:
    // Voice ID enum for indexing into voices[] array
    enum VoiceID {
        VOICE_IDLE, VOICE_REV, VOICE_START, VOICE_TURBO, VOICE_KNOCK,
        VOICE_WASTEGATE, VOICE_HORN, VOICE_SIREN, VOICE_BRAKE,
        VOICE_JAKE_BRAKE, VOICE_REVERSING, VOICE_PARKING_BRAKE,
        VOICE_SHIFTING, VOICE_INDICATOR, VOICE_COUPLING, VOICE_FAN,
        VOICE_SUPERCHARGER, VOICE_UNCOUPLING, VOICE_SOUND1,
        VOICE_COUNT
    };

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
    VoiceState voices[VOICE_COUNT];

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

    // Stop request
    bool engineStopRequested = false;
    int16_t lastThrottle = 0;

    // Timing
    uint32_t lastUpdateTime;
    uint32_t attenuatorMillis;
    uint8_t attenuator;
    float stopPitchFactor; // Pitch factor when stopping started

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

    // Helper: advance voice position with looping or one-shot behavior
    static inline void advanceVoice(VoiceState& v) {
        v.position += v.step;
        if (v.position >= (float)v.count) {
            if (v.loop) {
                v.position -= (float)v.count;
            } else {
                v.position = 0;
                v.active = false;
            }
        }
    }
};

#endif // RC_ENGINE_SOUND_H
