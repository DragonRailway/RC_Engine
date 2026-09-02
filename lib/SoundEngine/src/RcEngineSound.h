#ifndef RC_ENGINE_SOUND_H
#define RC_ENGINE_SOUND_H

#include <Arduino.h>
#include "SoundTypes.h"
#include "EngineSim.h"
#include "SoundSynth.h"

/**
 * @brief Unified Façade combining EngineSim and SoundSynth for backward compatibility.
 */
class RcEngineSound {
public:
    using EngineState = EngineSim::State;
    static constexpr EngineState OFF = EngineSim::OFF;
    static constexpr EngineState STARTING = EngineSim::STARTING;
    static constexpr EngineState RUNNING = EngineSim::RUNNING;
    static constexpr EngineState STOPPING = EngineSim::STOPPING;
    static constexpr EngineState PARKING_BRAKE = EngineSim::PARKING_BRAKE;

    using KnockPattern = SoundSynth::KnockPattern;
    static constexpr KnockPattern KNOCK_V8 = SoundSynth::KNOCK_V8;
    static constexpr KnockPattern KNOCK_V8_468 = SoundSynth::KNOCK_V8_468;
    static constexpr KnockPattern KNOCK_R6 = SoundSynth::KNOCK_R6;
    static constexpr KnockPattern KNOCK_R6_2 = SoundSynth::KNOCK_R6_2;
    static constexpr KnockPattern KNOCK_V2 = SoundSynth::KNOCK_V2;
    static constexpr KnockPattern KNOCK_UNIFORM = SoundSynth::KNOCK_UNIFORM;

    using TransmissionType = EngineSim::TransmissionType;
    static constexpr TransmissionType TRANS_NONE = EngineSim::TRANS_NONE;
    static constexpr TransmissionType TRANS_MANUAL = EngineSim::TRANS_MANUAL;
    static constexpr TransmissionType TRANS_AUTOMATIC = EngineSim::TRANS_AUTOMATIC;

    using VehicleType = EngineSim::VehicleType;
    static constexpr VehicleType VEHICLE_TRUCK = EngineSim::VEHICLE_TRUCK;
    static constexpr VehicleType VEHICLE_LOCOMOTIVE = EngineSim::VEHICLE_LOCOMOTIVE;
    static constexpr VehicleType VEHICLE_EXCAVATOR = EngineSim::VEHICLE_EXCAVATOR;
    static constexpr VehicleType VEHICLE_UNKNOWN = EngineSim::VEHICLE_UNKNOWN;

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

    RcEngineSound() = default;
    virtual ~RcEngineSound() = default;

    void begin(const SoundData& soundData, const Config& config);
    void begin(const SoundData& soundData);
    void setConfig(const Config& config);
    void applyVoiceVolumes() { m_synth.applyVoiceVolumes(); }

    void update(int16_t throttle);
    void update(const EngineSim::Input& input, uint32_t dtMs = 20);

    void startEngine() { m_sim.startEngine(); m_synth.syncState(m_sim); }
    void stopEngine() { m_sim.stopEngine(); m_synth.syncState(m_sim); }

    void triggerHorn(bool active) { m_synth.triggerHorn(active); }
    void triggerSiren(bool active) { m_synth.triggerSiren(active); }
    void triggerBrake(bool active) { m_synth.triggerBrake(active); }
    void triggerParkingBrake(bool active) { m_synth.triggerParkingBrake(active); }
    void triggerJakeBrake(bool active) { m_synth.triggerJakeBrake(active); }
    void triggerWastegate(bool active) { m_synth.triggerWastegate(active); }
    void triggerReversing(bool active) { m_synth.triggerReversing(active); }
    void triggerShifting(bool active) { m_synth.triggerShifting(active); }
    void triggerIndicator(bool active) { m_synth.triggerIndicator(active); }
    void triggerCoupling(bool active) { m_synth.triggerCoupling(active); }
    void triggerUncoupling(bool active) { m_synth.triggerUncoupling(active); }
    void triggerSound1(bool active) { m_synth.triggerSound1(active); }
    void triggerTireSqueal(bool active) { m_synth.triggerTireSqueal(active); }
    void triggerHydraulicPump(bool active) { m_synth.triggerHydraulicPump(active); }
    void triggerHydraulicFlow(bool active) { m_synth.triggerHydraulicFlow(active); }
    void triggerTrackRattle(bool active) { m_synth.triggerTrackRattle(active); }
    void triggerBucketRattle(bool active) { m_synth.triggerBucketRattle(active); }
    void triggerDumpBed(bool active) { m_synth.triggerDumpBed(active); }
    void triggerBell(bool active) { m_synth.triggerBell(active); }
    void triggerDoor(bool active) { m_synth.triggerDoor(active); }
    void triggerScanner(bool active) { m_synth.triggerScanner(active); }
    void triggerMusic(bool active) { m_synth.triggerMusic(active); }
    void triggerWhistle(bool active) { m_synth.triggerWhistle(active); }
    void triggerGun(bool active) { m_synth.triggerGun(active); }
    void triggerOutOfFuel(bool active) { m_synth.triggerOutOfFuel(active); }
    void triggerOthers(bool active) { m_synth.triggerOthers(active); }

    uint8_t getNextSample() {
        uint8_t sample = m_synth.getNextSample();
        if (m_sim.getState() == STARTING && m_synth.getState() == RUNNING) {
            m_sim.setRunning();
        }
        return sample;
    }
    void renderBlock(int16_t* interleavedStereoBuffer, size_t frames) {
        m_synth.renderBlock(interleavedStereoBuffer, frames);
        if (m_sim.getState() == STARTING && m_synth.getState() == RUNNING) {
            m_sim.setRunning();
        }
    }

    EngineState getState() const { return m_sim.getState(); }
    uint16_t getRpm() const { return m_sim.getRpm(); }
    uint8_t getGear() const { return m_sim.getGear(); }
    float getPitchFactor() const { return m_sim.getPitchFactor(); }
    float getCurrentThrottleFaded() const { return m_sim.getCurrentThrottleFaded(); }
    float getMotorSpeed() const { return m_sim.getMotorSpeed(); }
    bool isJakeBrakeActive() const { return m_sim.isJakeBrakeActive(); }

    EngineSim& getSim() { return m_sim; }
    SoundSynth& getSynth() { return m_synth; }
    const EngineSim& getSim() const { return m_sim; }
    const SoundSynth& getSynth() const { return m_synth; }

    static inline float readInterpolatedHermite4p(const int8_t* samples, uint32_t count, float position,
                                                  bool loop = true, uint32_t loopBegin = 0, uint32_t loopEnd = 0) {
        return SoundSynth::readInterpolatedHermite4p(samples, count, position, loop, loopBegin, loopEnd);
    }
    static inline int16_t saturateSoftKnee(float mixAccum) {
        return SoundSynth::saturateSoftKnee(mixAccum);
    }
    static inline int8_t readInterpolated(const int8_t* samples, uint32_t count, float position) {
        return (int8_t)SoundSynth::readInterpolatedHermite4p(samples, count, position);
    }

private:
    EngineSim m_sim;
    SoundSynth m_synth;
    Config cfg;
};

#endif // RC_ENGINE_SOUND_H
