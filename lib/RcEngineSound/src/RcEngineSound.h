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

    bool isDynamic = false; // Set to true if allocated in PSRAM/Heap
};

/**
 * @brief Core engine for RC sound and state simulation.
 */
class RcEngineSound {
public:
    enum EngineState {
        OFF,
        STARTING,
        RUNNING,
        STOPPING
    };

    struct Config {
        uint8_t acc = 2;
        uint8_t dec = 2;
        uint8_t inertia = 10; // 0-100, higher = slower RPM response
        uint16_t maxRpm = 500;
        uint16_t minRpm = 0;
        
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

        uint16_t revSwitchPoint = 50; // Points where rev sound starts to mix in
        uint16_t idleEndPoint = 300;   // Point where idle sound is 0%

        // Transmission & Clutch
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
    void begin(const SoundData& soundData); // Overload to avoid default arg issue
    void setConfig(const Config& config) { cfg = config; }

    void update(int16_t throttle);

    void startEngine();
    void stopEngine();
    void triggerHorn(bool active);

    uint8_t getNextSample();

    EngineState getState() const { return state; }
    uint16_t getRpm() const { return currentRpmFixed; }

private:
    EngineState state;
    SoundData sounds;
    Config cfg;

    int32_t currentRpm;
    uint16_t currentRpmFixed;
    
    uint32_t curEngineSample;
    uint32_t curRevSample;
    uint32_t curTurboSample;
    uint32_t curKnockSample;
    uint32_t curWastegateSample;
    uint32_t curStartSample;
    uint32_t curHornSample;
    
    int16_t lastThrottle;
    bool hornActive;
    bool wastegateTriggered;
    uint32_t wastegateTriggerMillis;
    bool engineStopRequested;
    
    uint32_t lastUpdateTime;
    uint32_t attenuatorMillis;
    uint8_t attenuator;
};

#endif // RC_ENGINE_SOUND_H
