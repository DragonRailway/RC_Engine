#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <RcEngineSound.h>
#include "SoundLoader.h"

struct VehicleProfile {
    SoundSlot* sounds[SOUND_COUNT] = {};
    RcEngineSound::Config config;
    char name[32] = {};
    bool loaded = false;

    static const char* soundTypeNames[];
    static const char* genericNames[];

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

    SoundSlot* getSound(SoundID idx) {
        if (idx >= SOUND_COUNT) return nullptr;
        return sounds[idx];
    }

    SoundSlot* getSound(const char* type) {
        for (int i = 0; i < SOUND_COUNT; i++) {
            if (strcmp(soundTypeNames[i], type) == 0) {
                return sounds[i];
            }
        }
        return nullptr;
    }

    void populateSoundData(SoundData& out) {
        out = SoundData();
        for (int i = 0; i < SOUND_COUNT; i++) {
            if (sounds[i]) {
                out.slots[i] = *sounds[i];
            }
        }
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

    void parseConfig(JsonDocument& doc, RcEngineSound::Config& cfg) {
        // Engine
        cfg.engine.acc = doc["ENGINE"]["ACCELERATION"] | 2;
        cfg.engine.dec = doc["ENGINE"]["DECELERATION"] | 1;
        cfg.engine.inertia = doc["ENGINE"]["INERTIA"] | 10;
        cfg.engine.maxRpm = 500;
        cfg.engine.minRpm = 0;
        cfg.engine.maxPitchFactor = doc["ENGINE"]["MAX_PITCH_FACTOR"] | 3.3f;
        cfg.engine.revSwitchPoint = doc["ENGINE"]["REV_SWITCH_POINT"] | 50;
        cfg.engine.idleEndPoint = doc["ENGINE"]["IDLE_END_POINT"] | 300;

        // Diesel knock
        String knockPatternStr = doc["ENGINE"]["KNOCK_PATTERN"] | "V8";
        if (knockPatternStr == "V8") cfg.engine.knockPattern = RcEngineSound::KNOCK_V8;
        else if (knockPatternStr == "V8_468") cfg.engine.knockPattern = RcEngineSound::KNOCK_V8_468;
        else if (knockPatternStr == "R6") cfg.engine.knockPattern = RcEngineSound::KNOCK_R6;
        else if (knockPatternStr == "R6_2") cfg.engine.knockPattern = RcEngineSound::KNOCK_R6_2;
        else if (knockPatternStr == "V2") cfg.engine.knockPattern = RcEngineSound::KNOCK_V2;
        else cfg.engine.knockPattern = RcEngineSound::KNOCK_UNIFORM;

        cfg.engine.knockInterval = doc["ENGINE"]["DIESEL_KNOCK_INTERVAL"] | 8;
        cfg.engine.knockAdaptiveVolume = doc["ENGINE"]["KNOCK_ADAPTIVE_VOLUME"] | 18;
        cfg.engine.minKnockVolume = doc["ENGINE"]["MIN_KNOCK_VOLUME"] | 80;
        cfg.engine.knockStartRpm = doc["ENGINE"]["KNOCK_START_RPM"] | 10;

        // Jake brake
        cfg.engine.jakeBrakeMinRpm = doc["ENGINE"]["JAKEBRAKE_MIN_RPM"] | 60;
        cfg.engine.jakeBrakeDecelRate = doc["ENGINE"]["JAKEBRAKE_DECEL_RATE"] | 5;

        // Supercharger
        cfg.engine.superchargerStartPoint = doc["ENGINE"]["SUPERCHARGER_START_POINT"] | 10;

        // Sound volumes
        cfg.sound.master = 100;
        cfg.sound.start = doc["SOUND_VOLUME"]["START"] | 100;
        cfg.sound.idle = doc["SOUND_VOLUME"]["IDLE"] | 100;
        cfg.sound.idleMin = doc["SOUND_VOLUME"]["IDLE_MIN"] | 0;
        cfg.sound.rev = doc["SOUND_VOLUME"]["REV"] | 100;
        cfg.sound.revMin = doc["SOUND_VOLUME"]["REV_MIN"] | 0;
        cfg.sound.fullThrottle = doc["SOUND_VOLUME"]["FULL_THROTTLE"] | 100;
        cfg.sound.turbo = doc["SOUND_VOLUME"]["TURBO"] | 0;
        cfg.sound.turboMin = doc["SOUND_VOLUME"]["TURBO_MIN"] | 0;
        cfg.sound.knock = doc["SOUND_VOLUME"]["KNOCK"] | 0;
        cfg.sound.knockMin = doc["SOUND_VOLUME"]["KNOCK_MIN"] | 0;
        cfg.sound.wastegate = doc["SOUND_VOLUME"]["WASTEGATE"] | 0;
        cfg.sound.wastegateMin = doc["SOUND_VOLUME"]["WASTEGATE_MIN"] | 0;
        cfg.sound.horn = doc["SOUND_VOLUME"]["HORN"] | 100;
        cfg.sound.fan = doc["SOUND_VOLUME"]["FAN"] | 0;
        cfg.sound.jakeBrake = doc["SOUND_VOLUME"]["JAKEBRAKE"] | 0;
        cfg.sound.jakeBrakeMin = doc["SOUND_VOLUME"]["JAKEBRAKE_MIN"] | 0;
        cfg.sound.shifting = doc["SOUND_VOLUME"]["SHIFTING"] | 0;
        cfg.sound.brake = doc["SOUND_VOLUME"]["BRAKE"] | 0;
        cfg.sound.reversing = doc["SOUND_VOLUME"]["REVERSING"] | 0;
        cfg.sound.siren = doc["SOUND_VOLUME"]["SIREN"] | 0;
        cfg.sound.parkingBrake = doc["SOUND_VOLUME"]["PARKING_BRAKE"] | 0;
        cfg.sound.supercharger = doc["SOUND_VOLUME"]["SUPERCHARGER"] | 0;
        cfg.sound.superchargerMin = doc["SOUND_VOLUME"]["SUPERCHARGER_MIN"] | 10;
        cfg.sound.indicator = doc["SOUND_VOLUME"]["INDICATOR"] | 0;
        cfg.sound.coupling = doc["SOUND_VOLUME"]["COUPLING"] | 0;
        cfg.sound.uncoupling = doc["SOUND_VOLUME"]["UNCOUPLING"] | 0;
        cfg.sound.sound1 = doc["SOUND_VOLUME"]["SOUND1"] | 100;
        cfg.sound.tireSqueal = doc["SOUND_VOLUME"]["TIRE_SQUEAL"] | 0;
        cfg.sound.hydraulicPump = doc["SOUND_VOLUME"]["HYDRAULIC_PUMP"] | 0;
        cfg.sound.hydraulicFlow = doc["SOUND_VOLUME"]["HYDRAULIC_FLOW"] | 0;
        cfg.sound.trackRattle = doc["SOUND_VOLUME"]["TRACK_RATTLE"] | 0;
        cfg.sound.bucketRattle = doc["SOUND_VOLUME"]["BUCKET_RATTLE"] | 0;
        // New voice channels
        cfg.sound.bell = doc["SOUND_VOLUME"]["BELL"] | 0;
        cfg.sound.door = doc["SOUND_VOLUME"]["DOOR"] | 0;
        cfg.sound.scanner = doc["SOUND_VOLUME"]["SCANNER"] | 0;
        cfg.sound.music = doc["SOUND_VOLUME"]["MUSIC"] | 0;
        cfg.sound.whistle = doc["SOUND_VOLUME"]["WHISTLE"] | 0;
        cfg.sound.gun = doc["SOUND_VOLUME"]["GUN"] | 0;
        cfg.sound.outOfFuel = doc["SOUND_VOLUME"]["OUT_OF_FUEL"] | 0;
        cfg.sound.others = doc["SOUND_VOLUME"]["OTHERS"] | 0;
        // Mix weights
        cfg.sound.engineMixWeight = doc["MIX_WEIGHTS"]["ENGINE"] | 100;
        cfg.sound.effectMixWeight = doc["MIX_WEIGHTS"]["EFFECTS"] | 100;
        cfg.sound.crawlerModeThreshold = doc["SOUND_VOLUME"]["CRAWLER_MODE_THRESHOLD"] | 44;

        // Transmission
        String transTypeStr = doc["TRANSMISSION"]["TYPE"] | "NONE";
        if (transTypeStr == "AUTOMATIC") cfg.transmission.type = RcEngineSound::TRANS_AUTOMATIC;
        else if (transTypeStr == "MANUAL") cfg.transmission.type = RcEngineSound::TRANS_MANUAL;
        else cfg.transmission.type = RcEngineSound::TRANS_NONE;

        cfg.transmission.numberOfGears = doc["TRANSMISSION"]["NUMBER_OF_GEARS"] | 3;

        JsonArray rampTimes = doc["TRANSMISSION"]["GEAR_RAMP_TIMES"].as<JsonArray>();
        if (rampTimes) {
            int idx = 0;
            for (int val : rampTimes) {
                if (idx < 6) cfg.transmission.gearRampTimes[idx++] = val;
            }
        }

        // Features
        cfg.features.hydraulicEnabled = doc["FEATURES"]["HYDRAULIC_ENABLED"] | false;
        cfg.features.hydrostaticMode = doc["FEATURES"]["HYDROSTATIC_MODE"] | false;
        cfg.features.trackRattleEnabled = doc["FEATURES"]["TRACK_RATTLE_ENABLED"] | false;
        cfg.features.dumpBedEnabled = doc["FEATURES"]["DUMP_BED_ENABLED"] | false;
        cfg.features.tireSquealThreshold = doc["FEATURES"]["TIRE_SQUEAL_THRESHOLD"] | 70;
        cfg.features.tireSquealMaxSpeed = doc["FEATURES"]["TIRE_SQUEAL_MAX_SPEED"] | 30;
        cfg.features.trackRattleIntervalMin = doc["FEATURES"]["TRACK_RATTLE_INTERVAL_MIN"] | 90;
        cfg.features.trackRattleIntervalMax = doc["FEATURES"]["TRACK_RATTLE_INTERVAL_MAX"] | 500;

        // Loop points
        cfg.loopPoints.hornBegin = doc["LOOP_POINTS"]["HORN_BEGIN"] | 0;
        cfg.loopPoints.hornEnd = doc["LOOP_POINTS"]["HORN_END"] | 0;
        cfg.loopPoints.sirenBegin = doc["LOOP_POINTS"]["SIREN_BEGIN"] | 0;
        cfg.loopPoints.sirenEnd = doc["LOOP_POINTS"]["SIREN_END"] | 0;
        cfg.loopPoints.reversingBegin = doc["LOOP_POINTS"]["REVERSING_BEGIN"] | 0;
        cfg.loopPoints.reversingEnd = doc["LOOP_POINTS"]["REVERSING_END"] | 0;
        cfg.loopPoints.sound1Begin = doc["LOOP_POINTS"]["SOUND1_BEGIN"] | 0;
        cfg.loopPoints.sound1End = doc["LOOP_POINTS"]["SOUND1_END"] | 0;
    }
};

const char* VehicleProfile::soundTypeNames[] = {
    "idle", "rev", "start", "knock", "turbo", "wastegate", "horn",
    "jakebrake", "fan", "siren", "airbrake", "parkingbrake",
    "shifting", "reversing", "indicator", "coupling", "supercharger",
    "uncoupling", "sound1", "tiresqueal", "hydraulicpump",
    "hydraulicflow", "trackrattle", "bucketrattle",
    "bell", "door", "scanner", "music", "whistle", "gun", "outoffuel", "others"
};

const char* VehicleProfile::genericNames[] = {
    "idle-ScaniaV8.json", "rev-ScaniaV8.json", "start-ScaniaV8.json",
    "knock-ScaniaV8.json", "Turbo-whistle.json", "1000HpScaniaV8-wastegate.json",
    "ScaniaV8train-horn.json",
    "ScaniaV8-jakebrake.json", "fan-Generic.json", "siren-Dummy.json",
    "airbrake-Truck2.json", "parkingbrake-Generic.json",
    "ClunkingGearShifting.json", "reversing-TruckBeep.json",
    "indicator-Generic.json", "coupling-generic.json", "supercharger.json",
    "uncoupling-generic.json", "sound1-Dummy.json",
    "squeal-Tire2.json", "hydraulicPump-Generic.json",
    "hydraulicFlow-Generic.json", "trackrattle.json", "bucketrattle-Generic.json",
    "bell-Dummy.json", "door.json", "scanner-kitt.json",
    "music-Dummy.json", "whistle-Turbo.json", "gun-Dummy.json",
    "outoffuel-Dummy.json", "others-Dummy.json"
};
