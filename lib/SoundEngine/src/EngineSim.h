#ifndef ENGINE_SIM_H
#define ENGINE_SIM_H

#include <Arduino.h>

/**
 * @brief Unified Powertrain and Engine Simulation Model.
 * 
 * Simulates:
 * - Engine state machine (OFF, STARTING, RUNNING, STOPPING, PARKING_BRAKE)
 * - RPM inertia & continuous slew-rate smoothing
 * - Automatic transmission with torque converter slip & gear shifting
 * - Manual transmission shifting points
 * - ESC motor speed ramping with inertia, acceleration, and dynamic braking
 * - Jake brake auto-triggering & deceleration
 * - Turbo wastegate blow-off detection
 * - Hydraulic pump governor & load offsets
 * - Tire squeal, track rattle, and crawler mode
 */
class EngineSim {
public:
    enum State {
        OFF,
        STARTING,
        RUNNING,
        STOPPING,
        PARKING_BRAKE
    };

    enum VehicleType {
        VEHICLE_TRUCK,
        VEHICLE_LOCOMOTIVE,
        VEHICLE_EXCAVATOR,
        VEHICLE_UNKNOWN
    };

    enum TransmissionType {
        TRANS_NONE,
        TRANS_MANUAL,
        TRANS_AUTOMATIC
    };

    struct Input {
        int16_t throttle = 0;     // Commanded throttle (-100 to 100, or scaled -maxRpm..maxRpm)
        int16_t brake = 0;        // Brake intensity (0 to 100%)
        uint8_t gear = 0;         // 0: Drive (or gear 1), 1: Park, 2: Reverse (or manual gear index)
        bool    parkingBrake = false;
        int16_t auxLoad = 0;      // Auxiliary hydraulic load offset (0 to 100%)
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
            uint8_t jakeBrakeMinRpm = 60;
            uint8_t jakeBrakeDecelRate = 5;
            uint8_t superchargerStartPoint = 10;
            uint16_t stopDuration = 1400;
        } engine;

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

        struct Sound {
            uint8_t master = 100;
            uint8_t crawlerModeThreshold = 44;
            uint8_t tireSqueal = 0;
            uint8_t hydraulicPump = 0;
            uint8_t trackRattle = 0;
            uint8_t jakeBrake = 0;
            uint8_t shifting = 0;
        } sound;
    };

    EngineSim();
    virtual ~EngineSim() = default;

    void begin(const Config& config);
    void setConfig(const Config& config);
    const Config& getConfig() const { return cfg; }

    void startEngine();
    void stopEngine();
    void setRunning() { if (state == STARTING) state = RUNNING; }

    // Primary 50 Hz simulation step
    void update(const Input& input, uint32_t dtMs = 20);

    // Overload for backward compatibility / direct throttle updating
    void update(int16_t throttle);

    // ── Outputs for Physical Hardware / VehicleController ──
    float   getMotorSpeed() const { return currentMotorSpeed; }
    State   getState() const { return state; }
    uint16_t getRpm() const { return currentRpmFixed; }
    float   getPitchFactor() const { return pitchFactor; }
    uint8_t getGear() const { return selectedGear; }
    float   getCurrentThrottleFaded() const { return currentThrottleFaded; }

    bool    isBrakeActive() const { return brakeActive; }
    bool    isReverseActive() const { return reverseActive; }
    bool    isJakeBrakeActive() const { return jakeBrakeActive; }
    bool    isWastegateTriggered() const { return wastegateTriggered; }
    bool    isGearShiftTriggered() const { return gearShiftTriggered; }
    bool    isBrakeSquealTriggered() const { return brakeSquealTriggered; }
    bool    isTireSquealActive() const { return tireSquealActive; }
    uint8_t getTireSquealVolume() const { return tireSquealVolume; }
    bool    isTrackRattleTriggered() const { return trackRattleTriggered; }
    bool    isHydraulicPumpActive() const { return hydraulicPumpActive; }
    uint8_t getHydraulicPumpVolume() const { return hydraulicPumpVolume; }

    // Direct event clears
    void    clearWastegateTrigger() { wastegateTriggered = false; }
    void    clearGearShiftTrigger() { gearShiftTriggered = false; }
    void    clearBrakeSquealTrigger() { brakeSquealTriggered = false; }
    void    clearTrackRattleTrigger() { trackRattleTriggered = false; }

private:
    Config cfg;
    State state = OFF;

    // RPM & pitch
    float currentRpm = 0.0f;
    uint16_t currentRpmFixed = 0;
    float pitchFactor = 1.0f;
    float currentThrottleFaded = 0.0f;

    // ESC Motor Speed Physics
    float currentMotorSpeed = 0.0f;
    uint32_t lastInertiaTime = 0;
    bool prevMotorMoving = false;

    // Precomputed invariants
    float invMaxRpm = 1.0f / 500.0f;
    float pitchRange = 0.4f;
    int32_t gearSize = 0;

    // Transmission & Speed
    uint8_t selectedGear = 1;
    uint8_t lastGear = 1;
    int32_t virtualSpeed = 0;

    // Events & Flags
    bool brakeActive = false;
    bool reverseActive = false;
    bool jakeBrakeActive = false;
    bool jakeBrakeRequest = false;
    bool wastegateTriggered = false;
    bool gearShiftTriggered = false;
    bool brakeSquealTriggered = false;
    bool tireSquealActive = false;
    uint8_t tireSquealVolume = 0;
    bool trackRattleTriggered = false;
    bool hydraulicPumpActive = false;
    uint8_t hydraulicPumpVolume = 0;

    // Timing & Stop State
    int16_t lastThrottle = 0;
    uint32_t lastUpdateTime = 0;
    uint32_t startStartMillis = 0;
    uint32_t stopStartMillis = 0;
    uint16_t stopDurationMs = 1400;
    float stopPitchFactor = 1.0f;
    uint32_t lastTrackRattleTime = 0;
    uint32_t wastegateTriggerMillis = 0;
    bool crawlerMode = false;
    bool engineStopRequested = false;
};

#endif // ENGINE_SIM_H
