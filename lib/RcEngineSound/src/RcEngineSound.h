#ifndef RC_ENGINE_SOUND_H
#define RC_ENGINE_SOUND_H

#include <Arduino.h>

/**
 * @brief Structure to hold engine sound samples and metadata.
 */
struct SoundData {
    const int8_t* samples; // Idle samples
    uint32_t sampleCount;
    uint16_t sampleRate;
    
    const int8_t* startSamples;
    uint32_t startSampleCount;
    
    const int8_t* revSamples = nullptr;
    uint32_t revSampleCount = 0;
    
    const int8_t* hornSamples = nullptr;
    uint32_t hornSampleCount = 0;
    uint16_t hornSampleRate = 8000;
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
        uint16_t maxRpm = 500;
        uint16_t minRpm = 0;
        
        uint8_t masterVolume = 100;
        uint8_t startVolume = 100;
        uint8_t idleVolume = 100;
        uint8_t revVolume = 100;
        uint8_t hornVolume = 100;

        uint16_t revSwitchPoint = 50; // Points where rev sound starts to mix in
        uint16_t idleEndPoint = 300;   // Point where idle sound is 0%
    };

    RcEngineSound();

    void begin(const SoundData& soundData, const Config& config);
    void begin(const SoundData& soundData); // Overload to avoid default arg issue

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
    uint32_t curStartSample;
    uint32_t curHornSample;
    
    bool hornActive;
    bool engineStopRequested;
    
    uint32_t lastUpdateTime;
    uint32_t attenuatorMillis;
    uint8_t attenuator;
};

#endif // RC_ENGINE_SOUND_H
