#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "PinMapper.h"
#include <RcEngineSound.h>
#include <SoundTypes.h>

class ConfigParser {
public:
    static bool begin() {
        if (!LittleFS.begin(true)) {
            Serial.println("[ConfigParser] LittleFS mount failed");
            return false;
        }
        Serial.println("[ConfigParser] LittleFS mounted");
        return true;
    }

    static bool loadHardwareConfig(const char* path, HardwareConfig& config) {
        File file = LittleFS.open(path, "r");
        if (!file) {
            Serial.printf("[ConfigParser] Cannot open: %s\n", path);
            return false;
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, file);
        file.close();

        if (error) {
            Serial.printf("[ConfigParser] JSON error in %s: %s\n", path, error.c_str());
            return false;
        }

        config = HardwareConfig();

        config = HardwareConfig();

        JsonObjectConst docObj = doc.as<JsonObjectConst>();

        if (!docObj["sound"].isNull() || !docObj["SOUND"].isNull()) {
            JsonObjectConst soundObj = docObj["sound"] | docObj["SOUND"];
            config.sound.volume = soundObj["volume"] | soundObj["VOLUME"] | 80;
        }

        if (!docObj["drivetrain"].isNull() || !docObj["DRIVE_MOTOR"].isNull()) {
            JsonObjectConst dtObj = docObj["drivetrain"];
            if (dtObj.isNull()) {
                // Legacy top-level DRIVE_MOTOR
                config.drivetrainType = HardwareConfig::ACKERMANN;
                parseDriveMotor(docObj["DRIVE_MOTOR"], config.driveMotor);
            } else if (!dtObj["left_motor"].isNull() || !dtObj["LEFT_MOTOR"].isNull()) {
                config.drivetrainType = HardwareConfig::SKID_STEER;
                parseDriveMotor(dtObj["left_motor"] | dtObj["LEFT_MOTOR"], config.leftMotor);
                parseDriveMotor(dtObj["right_motor"] | dtObj["RIGHT_MOTOR"], config.rightMotor);
                config.steeringSensitivity = dtObj["steering_sensitivity"] | dtObj["STEERING_SENSITIVITY"] | 80;
            } else {
                config.drivetrainType = HardwareConfig::ACKERMANN;
                JsonObjectConst driveObj = dtObj["drive_motor"] | dtObj["DRIVE_MOTOR"];
                if (!driveObj.isNull()) parseDriveMotor(driveObj, config.driveMotor);
                JsonObjectConst steerObj = dtObj["steering_servo"] | dtObj["STEERING_SERVO"];
                if (!steerObj.isNull()) parseSteeringServo(steerObj, config.steeringServo);
            }
        }

        if (!docObj["steering_servo"].isNull() || !docObj["STEERING_SERVO"].isNull()) {
            parseSteeringServo(docObj["steering_servo"] | docObj["STEERING_SERVO"], config.steeringServo);
        }

        if (!docObj["lights"].isNull() || !docObj["LIGHTS"].isNull()) {
            parseLights(docObj["lights"] | docObj["LIGHTS"], config.lights);
        }

        if (!docObj["telemetry"].isNull() || !docObj["TELEMETRY"].isNull()) {
            JsonObjectConst telObj = docObj["telemetry"] | docObj["TELEMETRY"];
            config.telemetry.vScale = telObj["voltage_scale"] | telObj["VOLTAGE_SCALE"] | 1.0f;
            config.telemetry.vOffset = telObj["voltage_offset"] | telObj["VOLTAGE_OFFSET"] | 0.0f;
        }

        Serial.printf("[ConfigParser] Loaded hardware config: %s\n", path);
        return true;
    }

    static bool loadVehicleConfig(const char* path, RcEngineSound::Config& config) {
        File file = LittleFS.open(path, "r");
        if (!file) {
            Serial.printf("[ConfigParser] Cannot open: %s\n", path);
            return false;
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, file);
        file.close();

        if (error) {
            Serial.printf("[ConfigParser] JSON error in %s: %s\n", path, error.c_str());
            return false;
        }

        config = RcEngineSound::Config();

        JsonObjectConst vehObj = doc["vehicle"] | doc["VEHICLE"];
        strlcpy(config.name, vehObj["name"] | vehObj["NAME"] | "Unknown", sizeof(config.name));
        strlcpy(config.soundSet, vehObj["sound_set"] | vehObj["SOUND_SET"] | config.name, sizeof(config.soundSet));
        strlcpy(config.preset, vehObj["preset"] | vehObj["PRESET"] | "generic", sizeof(config.preset));

        const char* vehicleTypeStr = vehObj["type"] | vehObj["TYPE"] | "TRUCK";
        if (strcasecmp(vehicleTypeStr, "LOCOMOTIVE") == 0) {
            config.type = RcEngineSound::VEHICLE_LOCOMOTIVE;
        } else if (strcasecmp(vehicleTypeStr, "EXCAVATOR") == 0) {
            config.type = RcEngineSound::VEHICLE_EXCAVATOR;
        } else if (strcasecmp(vehicleTypeStr, "CUSTOM") == 0) {
            config.type = RcEngineSound::VEHICLE_CUSTOM;
        } else {
            config.type = RcEngineSound::VEHICLE_TRUCK;
        }

        parseEngine(doc, config);
        parseSoundVolumes(doc, config);
        parseTransmission(doc, config);
        parseFeatures(doc, config);
        parseLoopPoints(doc, config);
        parseMixWeights(doc, config);

        Serial.printf("[ConfigParser] Loaded vehicle config: %s (sound_set=%s, preset=%s)\n",
                      config.name, config.soundSet, config.preset);
        return true;
    }

    static bool loadSounds(const RcEngineSound::Config& cfg, SoundData& soundData) {
        soundData = SoundData();

        String soundSet = cfg.soundSet[0] != '\0' ? cfg.soundSet : cfg.name;
        soundSet.replace(" ", "");
        String preset = cfg.preset[0] != '\0' ? cfg.preset : "generic";

        for (int i = 0; i < SOUND_COUNT; i++) {
            const char* slot = soundTypeNames[i];

            // Tier 1: /sounds/vehicles/<soundSet>/<slot>.json
            String p1 = "/sounds/vehicles/" + soundSet + "/" + String(slot) + ".json";
            soundData.slots[i] = loadSoundSlot(p1.c_str());

            // Tier 1 (flat legacy): /sounds/<soundSet>-<slot>.json
            if (!soundData.slots[i].samples) {
                String p1_flat = "/sounds/" + soundSet + "-" + String(slot) + ".json";
                soundData.slots[i] = loadSoundSlot(p1_flat.c_str());
            }

            // Tier 2: /sounds/presets/<preset>/<slot>.json
            if (!soundData.slots[i].samples) {
                String p2 = "/sounds/presets/" + preset + "/" + String(slot) + ".json";
                soundData.slots[i] = loadSoundSlot(p2.c_str());
            }

            // Tier 3: /sounds/generic/<slot>.json
            if (!soundData.slots[i].samples) {
                String p3 = "/sounds/generic/" + String(slot) + ".json";
                soundData.slots[i] = loadSoundSlot(p3.c_str());
            }

            // Tier 3 (flat legacy): /sounds/<slot>.json or /sounds/<genericNames[i]>
            if (!soundData.slots[i].samples) {
                String p3_flat = String("/sounds/") + slot + ".json";
                soundData.slots[i] = loadSoundSlot(p3_flat.c_str());
            }
            if (!soundData.slots[i].samples) {
                String legacyPath = String("/sounds/") + genericNames[i];
                soundData.slots[i] = loadSoundSlot(legacyPath.c_str());
            }
        }

        int loaded = 0;
        for (int i = 0; i < SOUND_COUNT; i++) {
            if (soundData.slots[i].samples) loaded++;
        }
        Serial.printf("[ConfigParser] Loaded %d sounds for %s (sound_set=%s, preset=%s)\n",
                      loaded, cfg.name, soundSet.c_str(), preset.c_str());
        return loaded > 0;
    }

    static bool loadSounds(const char* vehicleName, SoundData& soundData) {
        RcEngineSound::Config cfg;
        strlcpy(cfg.name, vehicleName, sizeof(cfg.name));
        strlcpy(cfg.soundSet, vehicleName, sizeof(cfg.soundSet));
        return loadSounds(cfg, soundData);
    }

    static void printFileTree(const char* dirPath = "/", int depth = 0) {
        File root = LittleFS.open(dirPath);
        if (!root || !root.isDirectory()) {
            Serial.printf("%*s(cannot open %s)\n", depth * 2, "", dirPath);
            return;
        }

        File entry = root.openNextFile();
        while (entry) {
            for (int i = 0; i < depth; i++) Serial.print("│ ");

            if (entry.isDirectory()) {
                Serial.printf("├─ 📁 %s/\n", entry.name());
                String subPath = String(dirPath);
                if (!subPath.endsWith("/")) subPath += "/";
                subPath += entry.name();
                printFileTree(subPath.c_str(), depth + 1);
            } else {
                Serial.printf("├─ 📄 %-35s  %7d bytes\n", entry.name(), entry.size());
            }
            entry = root.openNextFile();
        }
    }

    static void printFilesystemInfo() {
        Serial.println("\n╔══════════════════════════════════════════════════╗");
        Serial.println("║            LittleFS File Tree                    ║");
        Serial.println("╚══════════════════════════════════════════════════╝\n");

        printFileTree("/", 0);

        Serial.println();
        size_t total = LittleFS.totalBytes();
        size_t used  = LittleFS.usedBytes();
        Serial.println("──────────────────────────────────────────────────");
        Serial.printf("  Total: %d bytes  |  Used: %d bytes  |  Free: %d bytes\n",
                      total, used, total - used);
        Serial.printf("  Usage: %.1f%%\n", (float)used / total * 100.0f);
        Serial.println("──────────────────────────────────────────────────\n");
    }

    static const char* soundTypeNames[];
    static const char* genericNames[];

private:
    static SoundSlot loadSoundSlot(const char* path) {
        File file = LittleFS.open(path, "r");
        if (!file) return SoundSlot();

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, file);
        file.close();

        if (error) return SoundSlot();

        uint32_t sampleCount = doc["sampleCount"] | 0;
        uint16_t sampleRate = doc["sampleRate"] | 22050;
        JsonArray samples = doc["samples"].as<JsonArray>();

        if (sampleCount == 0 || !samples) return SoundSlot();

        int8_t* buffer = (int8_t*)ps_malloc(sampleCount);
        if (!buffer) return SoundSlot();

        uint32_t i = 0;
        for (int val : samples) {
            buffer[i++] = (int8_t)val;
        }

        SoundSlot slot;
        slot.samples = buffer;
        slot.sampleCount = sampleCount;
        slot.sampleRate = sampleRate;
        return slot;
    }

    static void parseDriveMotor(JsonVariantConst motor, HardwareConfig::DriveMotor& config) {
        if (motor.isNull()) return;
        const char* hw = motor["hardware"] | motor["HARDWARE"] | "";
        if (strcmp(hw, "HBRIDGE_A") == 0 || strcmp(hw, "HBRIDGE_B") == 0) {
            config.type = HardwareConfig::DriveMotor::HBRIDGE;
        } else if (hw[0] == 'S') {
            config.type = HardwareConfig::DriveMotor::ESC;
        }
        config.hardwareId = PinMapper::resolve(hw);
        config.frequency = motor["frequency"] | motor["FREQUENCY"] | 20000;

        const char* dir = motor["direction"] | motor["DIRECTION"] | "FORWARD";
        if (strcasecmp(dir, "REVERSE") == 0) config.direction = HardwareConfig::DriveMotor::REVERSE;
        else if (strcasecmp(dir, "UNI_FORWARD") == 0) config.direction = HardwareConfig::DriveMotor::UNI_FORWARD;
        else if (strcasecmp(dir, "UNI_REVERSE") == 0) config.direction = HardwareConfig::DriveMotor::UNI_REVERSE;
        else config.direction = HardwareConfig::DriveMotor::FORWARD;

        JsonVariantConst dutyObj = motor["duty"] | motor["DUTY"];
        config.duty.min = dutyObj["min"] | dutyObj["MIN"] | 20;
        config.duty.max = dutyObj["max"] | dutyObj["MAX"] | 90;
    }

    static void parseSteeringServo(JsonVariantConst servo, HardwareConfig::SteeringServo& config) {
        if (servo.isNull()) return;
        const char* hw = servo["hardware"] | servo["HARDWARE"] | "";
        config.hardwareId = PinMapper::resolve(hw);
        config.frequency = servo["frequency"] | servo["FREQUENCY"] | 50;

        JsonVariantConst epObj = servo["endpoints"] | servo["ENDPOINTS"];
        config.endpoints.left = epObj["left"] | epObj["LEFT"] | 1350;
        config.endpoints.right = epObj["right"] | epObj["RIGHT"] | 1650;
        config.endpoints.center = epObj["center"] | epObj["CENTER"] | 1500;
    }

    static void parseLights(JsonVariantConst lights, HardwareConfig::Lights& config) {
        if (lights.isNull()) return;

        JsonVariantConst head = lights["head_light"] | lights["HEAD_LIGHT"];
        if (!head.isNull()) {
            config.headLight.pin = PinMapper::resolve(head["hardware"] | head["HARDWARE"] | "");
            config.headLight.brightness = head["brightness_max"] | head["BRIGHTNESS_MAX"] | 60;
            config.headLight.configured = config.headLight.pin != 0xFF;
        }

        JsonVariantConst tail = lights["tail_light"] | lights["TAIL_LIGHT"];
        if (!tail.isNull()) {
            config.tailLight.pin = PinMapper::resolve(tail["hardware"] | tail["HARDWARE"] | "");
            config.tailLight.brightness = tail["brightness_max"] | tail["BRIGHTNESS_MAX"] | 60;
            config.tailLight.configured = config.tailLight.pin != 0xFF;
        }

        JsonVariantConst brake = lights["brake_light"] | lights["BRAKE_LIGHT"];
        if (!brake.isNull()) {
            config.brakeLight.pin = PinMapper::resolve(brake["hardware"] | brake["HARDWARE"] | "");
            config.brakeLight.brightness = 100;
            config.brakeLight.configured = config.brakeLight.pin != 0xFF;
        }

        JsonVariantConst turn = lights["turn_light"] | lights["TURN_LIGHT"];
        if (!turn.isNull()) {
            JsonVariantConst turnL = turn["left"] | turn["LEFT"];
            JsonVariantConst turnR = turn["right"] | turn["RIGHT"];
            config.turnLight.leftPin = PinMapper::resolve(turnL["hardware"] | turnL["HARDWARE"] | turn["left_hardware"] | "");
            config.turnLight.rightPin = PinMapper::resolve(turnR["hardware"] | turnR["HARDWARE"] | turn["right_hardware"] | "");
            config.turnLight.brightness = turn["brightness_max"] | turn["BRIGHTNESS_MAX"] | 60;
            config.turnLight.intervalOn = turn["interval_on"] | turn["INTERVAL_ON"] | 500;
            config.turnLight.intervalOff = turn["interval_off"] | turn["INTERVAL_OFF"] | 500;
            config.turnLight.configured = config.turnLight.leftPin != 0xFF || config.turnLight.rightPin != 0xFF;
        }

        JsonVariantConst reversing = lights["reversing_light"] | lights["REVERSING_LIGHT"];
        if (!reversing.isNull()) {
            const char* hw = reversing["hardware"] | reversing["HARDWARE"] | "";
            config.reversingLight.pin = PinMapper::resolve(hw);
            if (config.reversingLight.pin == 0xFF) {
                if ((strcasecmp(hw, "head_light") == 0 || strcasecmp(hw, "HEAD_LIGHT") == 0) && config.headLight.configured) config.reversingLight.pin = config.headLight.pin;
                else if ((strcasecmp(hw, "tail_light") == 0 || strcasecmp(hw, "TAIL_LIGHT") == 0) && config.tailLight.configured) config.reversingLight.pin = config.tailLight.pin;
                else if ((strcasecmp(hw, "brake_light") == 0 || strcasecmp(hw, "BRAKE_LIGHT") == 0) && config.brakeLight.configured) config.reversingLight.pin = config.brakeLight.pin;
            }
            config.reversingLight.brightness = 100;
            config.reversingLight.configured = config.reversingLight.pin != 0xFF;
        }
    }

    static void parseEngine(JsonDocument& doc, RcEngineSound::Config& cfg) {
        JsonVariantConst eng = doc["engine"] | doc["ENGINE"];
        cfg.engine.acc = eng["acceleration"] | eng["ACCELERATION"] | 2;
        cfg.engine.dec = eng["deceleration"] | eng["DECELERATION"] | 1;
        cfg.engine.inertia = eng["inertia"] | eng["INERTIA"] | 10;
        cfg.engine.maxRpm = 500;
        cfg.engine.minRpm = 0;
        cfg.engine.maxPitchFactor = eng["max_pitch_factor"] | eng["MAX_PITCH_FACTOR"] | 3.3f;
        cfg.engine.revSwitchPoint = eng["rev_switch_point"] | eng["REV_SWITCH_POINT"] | 50;
        cfg.engine.idleEndPoint = eng["idle_end_point"] | eng["IDLE_END_POINT"] | 300;

        const char* knockStr = eng["knock_pattern"] | eng["KNOCK_PATTERN"] | "v8";
        if (strcasecmp(knockStr, "v8") == 0 || strcasecmp(knockStr, "V8") == 0) cfg.engine.knockPattern = RcEngineSound::KNOCK_V8;
        else if (strcasecmp(knockStr, "v8_468") == 0 || strcasecmp(knockStr, "V8_468") == 0) cfg.engine.knockPattern = RcEngineSound::KNOCK_V8_468;
        else if (strcasecmp(knockStr, "r6") == 0 || strcasecmp(knockStr, "R6") == 0) cfg.engine.knockPattern = RcEngineSound::KNOCK_R6;
        else if (strcasecmp(knockStr, "r6_2") == 0 || strcasecmp(knockStr, "R6_2") == 0) cfg.engine.knockPattern = RcEngineSound::KNOCK_R6_2;
        else if (strcasecmp(knockStr, "v2") == 0 || strcasecmp(knockStr, "V2") == 0) cfg.engine.knockPattern = RcEngineSound::KNOCK_V2;
        else cfg.engine.knockPattern = RcEngineSound::KNOCK_UNIFORM;

        cfg.engine.knockInterval = eng["diesel_knock_interval"] | eng["DIESEL_KNOCK_INTERVAL"] | 8;
        cfg.engine.knockAdaptiveVolume = eng["knock_adaptive_volume"] | eng["KNOCK_ADAPTIVE_VOLUME"] | 18;
        cfg.engine.minKnockVolume = eng["min_knock_volume"] | eng["MIN_KNOCK_VOLUME"] | 80;
        cfg.engine.knockStartRpm = eng["knock_start_rpm"] | eng["KNOCK_START_RPM"] | 10;

        cfg.engine.jakeBrakeMinRpm = eng["jakebrake_min_rpm"] | eng["JAKEBRAKE_MIN_RPM"] | 60;
        cfg.engine.jakeBrakeDecelRate = eng["jakebrake_decel_rate"] | eng["JAKEBRAKE_DECEL_RATE"] | 5;

        cfg.engine.superchargerStartPoint = eng["supercharger_start_point"] | eng["SUPERCHARGER_START_POINT"] | 10;
    }

    static void parseSoundVolumes(JsonDocument& doc, RcEngineSound::Config& cfg) {
        JsonVariantConst sv = doc["sound_volumes"] | doc["SOUND_VOLUME"];
        cfg.sound.master = 100;
        cfg.sound.start = sv["start"] | sv["START"] | 100;
        cfg.sound.idle = sv["idle"] | sv["IDLE"] | 100;
        cfg.sound.idleMin = sv["idle_min"] | sv["IDLE_MIN"] | 0;
        cfg.sound.rev = sv["rev"] | sv["REV"] | 100;
        cfg.sound.revMin = sv["rev_min"] | sv["REV_MIN"] | 0;
        cfg.sound.fullThrottle = sv["full_throttle"] | sv["FULL_THROTTLE"] | 100;
        cfg.sound.turbo = sv["turbo"] | sv["TURBO"] | 0;
        cfg.sound.turboMin = sv["turbo_min"] | sv["TURBO_MIN"] | 0;
        cfg.sound.knock = sv["knock"] | sv["KNOCK"] | 0;
        cfg.sound.knockMin = sv["knock_min"] | sv["KNOCK_MIN"] | 0;
        cfg.sound.wastegate = sv["wastegate"] | sv["WASTEGATE"] | 0;
        cfg.sound.wastegateMin = sv["wastegate_min"] | sv["WASTEGATE_MIN"] | 0;
        cfg.sound.horn = sv["horn"] | sv["HORN"] | 100;
        cfg.sound.fan = sv["fan"] | sv["FAN"] | 0;
        cfg.sound.jakeBrake = sv["jakebrake"] | sv["JAKEBRAKE"] | 0;
        cfg.sound.jakeBrakeMin = sv["jakebrake_min"] | sv["JAKEBRAKE_MIN"] | 0;
        cfg.sound.shifting = sv["shifting"] | sv["SHIFTING"] | 0;
        cfg.sound.brake = sv["brake"] | sv["BRAKE"] | 0;
        cfg.sound.reversing = sv["reversing"] | sv["REVERSING"] | 0;
        cfg.sound.siren = sv["siren"] | sv["SIREN"] | 0;
        cfg.sound.parkingBrake = sv["parking_brake"] | sv["PARKING_BRAKE"] | 0;
        cfg.sound.supercharger = sv["supercharger"] | sv["SUPERCHARGER"] | 0;
        cfg.sound.superchargerMin = sv["supercharger_min"] | sv["SUPERCHARGER_MIN"] | 10;
        cfg.sound.indicator = sv["indicator"] | sv["INDICATOR"] | 0;
        cfg.sound.coupling = sv["coupling"] | sv["COUPLING"] | 0;
        cfg.sound.uncoupling = sv["uncoupling"] | sv["UNCOUPLING"] | 0;
        cfg.sound.sound1 = sv["sound1"] | sv["SOUND1"] | 100;
        cfg.sound.tireSqueal = sv["tire_squeal"] | sv["TIRE_SQUEAL"] | 0;
        cfg.sound.hydraulicPump = sv["hydraulic_pump"] | sv["HYDRAULIC_PUMP"] | 0;
        cfg.sound.hydraulicFlow = sv["hydraulic_flow"] | sv["HYDRAULIC_FLOW"] | 0;
        cfg.sound.trackRattle = sv["track_rattle"] | sv["TRACK_RATTLE"] | 0;
        cfg.sound.bucketRattle = sv["bucket_rattle"] | sv["BUCKET_RATTLE"] | 0;
        cfg.sound.bell = sv["bell"] | sv["BELL"] | 0;
        cfg.sound.door = sv["door"] | sv["DOOR"] | 0;
        cfg.sound.scanner = sv["scanner"] | sv["SCANNER"] | 0;
        cfg.sound.music = sv["music"] | sv["MUSIC"] | 0;
        cfg.sound.whistle = sv["whistle"] | sv["WHISTLE"] | 0;
        cfg.sound.gun = sv["gun"] | sv["GUN"] | 0;
        cfg.sound.outOfFuel = sv["out_of_fuel"] | sv["OUT_OF_FUEL"] | 0;
        cfg.sound.others = sv["others"] | sv["OTHERS"] | 0;
        
        JsonVariantConst mw = doc["mix_weights"] | doc["MIX_WEIGHTS"];
        cfg.sound.engineMixWeight = mw["engine"] | mw["ENGINE"] | 100;
        cfg.sound.effectMixWeight = mw["effects"] | mw["EFFECTS"] | 100;
        cfg.sound.crawlerModeThreshold = sv["crawler_mode_threshold"] | sv["CRAWLER_MODE_THRESHOLD"] | 44;
    }

    static void parseTransmission(JsonDocument& doc, RcEngineSound::Config& cfg) {
        JsonVariantConst tr = doc["transmission"] | doc["TRANSMISSION"];
        const char* transTypeStr = tr["type"] | tr["TYPE"] | "NONE";
        if (strcasecmp(transTypeStr, "AUTOMATIC") == 0) cfg.transmission.type = RcEngineSound::TRANS_AUTOMATIC;
        else if (strcasecmp(transTypeStr, "MANUAL") == 0) cfg.transmission.type = RcEngineSound::TRANS_MANUAL;
        else cfg.transmission.type = RcEngineSound::TRANS_NONE;

        cfg.transmission.numberOfGears = tr["number_of_gears"] | tr["NUMBER_OF_GEARS"] | 3;

        JsonArrayConst rampTimes = tr["gear_ramp_times"] | tr["GEAR_RAMP_TIMES"];
        if (rampTimes) {
            int idx = 0;
            for (int val : rampTimes) {
                if (idx < 6) cfg.transmission.gearRampTimes[idx++] = val;
            }
        }
    }

    static void parseFeatures(JsonDocument& doc, RcEngineSound::Config& cfg) {
        JsonVariantConst ft = doc["features"] | doc["FEATURES"];
        cfg.features.hydraulicEnabled = ft["hydraulic_enabled"] | ft["HYDRAULIC_ENABLED"] | false;
        cfg.features.hydrostaticMode = ft["hydrostatic_mode"] | ft["HYDROSTATIC_MODE"] | false;
        cfg.features.trackRattleEnabled = ft["track_rattle_enabled"] | ft["TRACK_RATTLE_ENABLED"] | false;
        cfg.features.dumpBedEnabled = ft["dump_bed_enabled"] | ft["DUMP_BED_ENABLED"] | false;
        cfg.features.tireSquealThreshold = ft["tire_squeal_threshold"] | ft["TIRE_SQUEAL_THRESHOLD"] | 70;
        cfg.features.tireSquealMaxSpeed = ft["tire_squeal_max_speed"] | ft["TIRE_SQUEAL_MAX_SPEED"] | 30;
        cfg.features.trackRattleIntervalMin = ft["track_rattle_interval_min"] | ft["TRACK_RATTLE_INTERVAL_MIN"] | 90;
        cfg.features.trackRattleIntervalMax = ft["track_rattle_interval_max"] | ft["TRACK_RATTLE_INTERVAL_MAX"] | 500;
    }

    static void parseLoopPoints(JsonDocument& doc, RcEngineSound::Config& cfg) {
        JsonVariantConst lp = doc["loop_points"] | doc["LOOP_POINTS"];
        cfg.loopPoints.hornBegin = lp["horn_begin"] | lp["HORN_BEGIN"] | 0;
        cfg.loopPoints.hornEnd = lp["horn_end"] | lp["HORN_END"] | 0;
        cfg.loopPoints.sirenBegin = lp["siren_begin"] | lp["SIREN_BEGIN"] | 0;
        cfg.loopPoints.sirenEnd = lp["siren_end"] | lp["SIREN_END"] | 0;
        cfg.loopPoints.reversingBegin = lp["reversing_begin"] | lp["REVERSING_BEGIN"] | 0;
        cfg.loopPoints.reversingEnd = lp["reversing_end"] | lp["REVERSING_END"] | 0;
        cfg.loopPoints.sound1Begin = lp["sound1_begin"] | lp["SOUND1_BEGIN"] | 0;
        cfg.loopPoints.sound1End = lp["sound1_end"] | lp["SOUND1_END"] | 0;
    }

    static void parseMixWeights(JsonDocument& doc, RcEngineSound::Config& cfg) {
        JsonVariantConst mw = doc["mix_weights"] | doc["MIX_WEIGHTS"];
        cfg.sound.engineMixWeight = mw["engine"] | mw["ENGINE"] | 100;
        cfg.sound.effectMixWeight = mw["effects"] | mw["EFFECTS"] | 100;
    }
};

const char* ConfigParser::soundTypeNames[] = {
    "idle", "rev", "start", "knock", "turbo", "wastegate", "horn",
    "jakebrake", "fan", "siren", "airbrake", "parkingbrake",
    "shifting", "reversing", "indicator", "coupling", "supercharger",
    "uncoupling", "sound1", "tiresqueal", "hydraulicpump",
    "hydraulicflow", "trackrattle", "bucketrattle",
    "bell", "door", "scanner", "music", "whistle", "gun", "outoffuel", "others"
};

const char* ConfigParser::genericNames[] = {
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
