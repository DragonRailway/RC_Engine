#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <RcEngineSound.h>
#include "SoundLoader.h"

struct VehicleProfile {
    SoundData* sounds[20] = {};  // All loaded sounds (increased for uncoupling, sound1)
    RcEngineSound::Config config;
    char name[32] = {};
    bool loaded = false;

    // Sound indices
    enum SoundIndex {
        IDLE, REV, START, KNOCK, TURBO, WASTEGATE, HORN,
        JAKE_BRAKE, FAN, SIREN, BRAKE, PARKING_BRAKE,
        SHIFTING, REVERSING, INDICATOR, COUPLING, SUPERCHARGER,
        UNCOUPLING, SOUND1
    };

    static const char* soundTypeNames[];
    static const char* genericNames[];
    static const int SOUND_COUNT = 19;

    bool load(const char* profilePath) {
        unload();

        File file = LittleFS.open(profilePath, "r");
        if (!file) {
            Serial.printf("[VehicleProfile] Cannot open: %s\n", profilePath);
            return false;
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, file);
        file.close();

        if (error) {
            Serial.printf("[VehicleProfile] JSON error: %s\n", error.c_str());
            return false;
        }

        strlcpy(name, doc["VEHICLE"]["NAME"] | "Unknown", sizeof(name));
        Serial.printf("[VehicleProfile] Loading: %s\n", name);

        parseConfig(doc, config);

        String basePath = "/sounds/";
        String vehicleName = name;
        vehicleName.replace(" ", "");

        for (int i = 0; i < SOUND_COUNT; i++) {
            String vehiclePath = basePath + vehicleName + "-" + soundTypeNames[i] + ".json";
            String genericPath = basePath + genericNames[i];

            sounds[i] = SoundLoader::loadSound(vehiclePath.c_str());
            if (!sounds[i]) {
                sounds[i] = SoundLoader::loadSound(genericPath.c_str());
            }
        }

        loaded = true;
        Serial.printf("[VehicleProfile] Loaded %s with %d sounds\n", name, countLoaded());
        return true;
    }

    void unload() {
        SoundLoader::unloadAll(sounds, SOUND_COUNT);
        loaded = false;
        name[0] = '\0';
    }

    SoundData* getSound(SoundIndex idx) {
        if (idx < 0 || idx >= SOUND_COUNT) return nullptr;
        return sounds[idx];
    }

    SoundData* getSound(const char* type) {
        for (int i = 0; i < SOUND_COUNT; i++) {
            if (strcmp(soundTypeNames[i], type) == 0) {
                return sounds[i];
            }
        }
        return nullptr;
    }

    void populateSoundData(SoundData& out) {
        out = SoundData();
        out.samples = getSampleAndCount(IDLE, out.sampleCount);
        out.revSamples = getSampleAndCount(REV, out.revSampleCount);
        out.startSamples = getSampleAndCount(START, out.startSampleCount);
        out.knockSamples = getSampleAndCount(KNOCK, out.knockSampleCount);
        out.turboSamples = getSampleAndCount(TURBO, out.turboSampleCount);
        out.wastegateSamples = getSampleAndCount(WASTEGATE, out.wastegateSampleCount);
        out.hornSamples = getSampleAndCount(HORN, out.hornSampleCount);
        out.hornSampleRate = getSampleRate(HORN);
        out.jakeBrakeSamples = getSampleAndCount(JAKE_BRAKE, out.jakeBrakeSampleCount);
        out.fanSamples = getSampleAndCount(FAN, out.fanSampleCount);
        out.sirenSamples = getSampleAndCount(SIREN, out.sirenSampleCount);
        out.brakeSamples = getSampleAndCount(BRAKE, out.brakeSampleCount);
        out.reversingSamples = getSampleAndCount(REVERSING, out.reversingSampleCount);
        out.parkingBrakeSamples = getSampleAndCount(PARKING_BRAKE, out.parkingBrakeSampleCount);
        out.superchargerSamples = getSampleAndCount(SUPERCHARGER, out.superchargerSampleCount);
        out.shiftingSamples = getSampleAndCount(SHIFTING, out.shiftingSampleCount);
        out.indicatorSamples = getSampleAndCount(INDICATOR, out.indicatorSampleCount);
        out.couplingSamples = getSampleAndCount(COUPLING, out.couplingSampleCount);
        out.uncouplingSamples = getSampleAndCount(UNCOUPLING, out.uncouplingSampleCount);
        out.sound1Samples = getSampleAndCount(SOUND1, out.sound1SampleCount);
    }

    void populateConfig(RcEngineSound::Config& out) {
        out = config;
    }

private:
    int countLoaded() {
        int count = 0;
        for (int i = 0; i < SOUND_COUNT; i++) {
            if (sounds[i]) count++;
        }
        return count;
    }

    int8_t* getSampleAndCount(SoundIndex idx, uint32_t& count) {
        SoundData* sd = sounds[idx];
        if (sd) {
            count = sd->sampleCount;
            return sd->samples;
        }
        count = 0;
        return nullptr;
    }

    uint16_t getSampleRate(SoundIndex idx) {
        SoundData* sd = sounds[idx];
        return sd ? sd->sampleRate : 22050;
    }

    void parseConfig(JsonDocument& doc, RcEngineSound::Config& cfg) {
        // Engine behavior
        cfg.acc = doc["ENGINE"]["ACCELERATION"] | 2;
        cfg.dec = doc["ENGINE"]["DECELERATION"] | 1;
        cfg.inertia = doc["ENGINE"]["INERTIA"] | 10;
        cfg.maxRpm = 500;
        cfg.minRpm = 0;

        // Sound volumes
        cfg.masterVolume = 100;
        cfg.startVolume = doc["SOUND_VOLUME"]["START"] | 100;
        cfg.idleVolume = doc["SOUND_VOLUME"]["IDLE"] | 100;
        cfg.revVolume = doc["SOUND_VOLUME"]["REV"] | 100;
        cfg.turboVolume = doc["SOUND_VOLUME"]["TURBO"] | 0;
        cfg.knockVolume = doc["SOUND_VOLUME"]["KNOCK"] | 0;
        cfg.wastegateVolume = doc["SOUND_VOLUME"]["WASTEGATE"] | 0;
        cfg.hornVolume = doc["SOUND_VOLUME"]["HORN"] | 100;
        cfg.fanVolume = doc["SOUND_VOLUME"]["FAN"] | 0;
        cfg.jakeBrakeVolume = doc["SOUND_VOLUME"]["JAKEBRAKE"] | 0;
        cfg.shiftingVolume = doc["SOUND_VOLUME"]["SHIFTING"] | 0;
        cfg.brakeVolume = doc["SOUND_VOLUME"]["BRAKE"] | 0;
        cfg.reversingVolume = doc["SOUND_VOLUME"]["REVERSING"] | 0;
        cfg.sirenVolume = doc["SOUND_VOLUME"]["SIREN"] | 0;
        cfg.parkingBrakeVolume = doc["SOUND_VOLUME"]["PARKING_BRAKE"] | 0;
        cfg.superchargerVolume = doc["SOUND_VOLUME"]["SUPERCHARGER"] | 0;
        cfg.indicatorVolume = doc["SOUND_VOLUME"]["INDICATOR"] | 0;
        cfg.couplingVolume = doc["SOUND_VOLUME"]["COUPLING"] | 0;
        cfg.uncouplingVolume = doc["SOUND_VOLUME"]["UNCOUPLING"] | 0;
        cfg.sound1Volume = doc["SOUND_VOLUME"]["SOUND1"] | 100;

        // Pitch shifting
        cfg.maxPitchFactor = doc["ENGINE"]["MAX_PITCH_FACTOR"] | 3.3f;

        // Idle/Rev cross-fade
        cfg.revSwitchPoint = doc["ENGINE"]["REV_SWITCH_POINT"] | 50;
        cfg.idleEndPoint = doc["ENGINE"]["IDLE_END_POINT"] | 300;

        // Diesel knock
        String knockPatternStr = doc["ENGINE"]["KNOCK_PATTERN"] | "V8";
        if (knockPatternStr == "V8") cfg.knockPattern = RcEngineSound::KNOCK_V8;
        else if (knockPatternStr == "V8_468") cfg.knockPattern = RcEngineSound::KNOCK_V8_468;
        else if (knockPatternStr == "R6") cfg.knockPattern = RcEngineSound::KNOCK_R6;
        else if (knockPatternStr == "R6_2") cfg.knockPattern = RcEngineSound::KNOCK_R6_2;
        else if (knockPatternStr == "V2") cfg.knockPattern = RcEngineSound::KNOCK_V2;
        else cfg.knockPattern = RcEngineSound::KNOCK_UNIFORM;

        cfg.knockInterval = doc["ENGINE"]["DIESEL_KNOCK_INTERVAL"] | 8;
        cfg.knockAdaptiveVolume = doc["ENGINE"]["KNOCK_ADAPTIVE_VOLUME"] | 18;

        // Jake brake
        cfg.jakeBrakeMinRpm = doc["ENGINE"]["JAKEBRAKE_MIN_RPM"] | 60;
        cfg.jakeBrakeDecelRate = doc["ENGINE"]["JAKEBRAKE_DECEL_RATE"] | 5;

        // Supercharger
        cfg.superchargerStartPoint = doc["ENGINE"]["SUPERCHARGER_START_POINT"] | 10;

        // Transmission
        String transTypeStr = doc["TRANSMISSION"]["TYPE"] | "NONE";
        if (transTypeStr == "AUTOMATIC") cfg.transmissionType = RcEngineSound::TRANS_AUTOMATIC;
        else if (transTypeStr == "MANUAL") cfg.transmissionType = RcEngineSound::TRANS_MANUAL;
        else cfg.transmissionType = RcEngineSound::TRANS_NONE;

        cfg.numberOfGears = doc["TRANSMISSION"]["NUMBER_OF_GEARS"] | 3;
        cfg.automatic = (cfg.transmissionType == RcEngineSound::TRANS_AUTOMATIC);

        // Gear ramp times
        JsonArray rampTimes = doc["TRANSMISSION"]["GEAR_RAMP_TIMES"].as<JsonArray>();
        if (rampTimes) {
            int idx = 0;
            for (int val : rampTimes) {
                if (idx < 6) cfg.gearRampTimes[idx++] = val;
            }
        }

        // Legacy
        cfg.clutchEngagingPoint = doc["ENGINE"]["CLUTCH_RPM"] | 100;
        cfg.maxRpmPercentage = 310;
    }
};

const char* VehicleProfile::soundTypeNames[] = {
    "idle", "rev", "start", "knock", "turbo", "wastegate", "horn",
    "jakebrake", "fan", "siren", "airbrake", "parkingbrake",
    "shifting", "reversing", "indicator", "coupling", "supercharger",
    "uncoupling", "sound1"
};

const char* VehicleProfile::genericNames[] = {
    "idle-ScaniaV8.json", "rev-ScaniaV8.json", "start-ScaniaV8.json",
    "knock-ScaniaV8.json", "Turbo-whistle.json", "1000HpScaniaV8-wastegate.json",
    "ScaniaV8train-horn.json",
    "ScaniaV8-jakebrake.json", "fan-Generic.json", "siren-Dummy.json",
    "airbrake-Truck2.json", "parkingbrake-Generic.json",
    "ClunkingGearShifting.json", "reversing-TruckBeep.json",
    "indicator-Generic.json", "coupling-generic.json", "supercharger.json",
    "uncoupling-generic.json", "sound1-Dummy.json"
};
