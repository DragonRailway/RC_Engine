#include "ConfigParser.h"
#include "boards.h"
#include <RadioKitLib.h>

// ── Static member definitions ───────────────────────────────────────────────
const char* ConfigParser::soundTypeNames[] = {
    "idle", "rev", "start", "knock", "turbo", "wastegate", "horn",
    "jakebrake", "fan", "siren", "airbrake", "parkingbrake",
    "shifting", "reversing", "indicator", "coupling", "supercharger",
    "uncoupling", "sound1", "tiresqueal", "hydraulicpump",
    "hydraulicflow", "trackrattle", "bucketrattle",
    "bell", "door", "scanner", "music", "whistle", "gun", "outoffuel", "others"
};

// ── Public API methods ──────────────────────────────────────────────────────

bool ConfigParser::begin() {
    RKFs::begin();
    if (!RKFs::isReady()) {
        Serial.println("[ConfigParser] LittleFS mount failed");
        return false;
    }
    Serial.println("[ConfigParser] LittleFS mounted");
    return true;
}

String ConfigParser::findHardwareConfig() {
    File root = LittleFS.open("/");
    if (!root || !root.isDirectory()) return "";

    File entry = root.openNextFile();
    while (entry) {
        if (!entry.isDirectory()) {
            const char* rawName = entry.name();
            const char* p = rawName;
            if (p[0] == '/') p++;
            const char* slash = strrchr(p, '/');
            if (slash) p = slash + 1;
            if (strncmp(p, "hardware-", 9) == 0 && String(p).endsWith(".json")) {
                String res = String("/") + p;
                entry.close();
                root.close();
                return res;
            }
        }
        entry = root.openNextFile();
    }
    root.close();
    return "";
}

String ConfigParser::findVehicleConfig() {
    // 1. Search root directory
    File root = LittleFS.open("/");
    if (root && root.isDirectory()) {
        File entry = root.openNextFile();
        while (entry) {
            if (!entry.isDirectory()) {
                const char* rawName = entry.name();
                const char* p = rawName;
                if (p[0] == '/') p++;
                const char* slash = strrchr(p, '/');
                if (slash) p = slash + 1;
                if (strcmp(p, "vehicle.json") == 0 ||
                    (strncmp(p, "vehicle-", 8) == 0 && String(p).endsWith(".json"))) {
                    String res = String("/") + p;
                    entry.close();
                    root.close();
                    return res;
                }
            }
            entry = root.openNextFile();
        }
        root.close();
    }

    // 2. Search /vehicle_configs and /vehicle_config directories
    const char* subdirs[] = {"/vehicle_configs", "/vehicle_config"};
    for (const char* dirName : subdirs) {
        File dir = LittleFS.open(dirName);
        if (dir && dir.isDirectory()) {
            File entry = dir.openNextFile();
            while (entry) {
                if (entry.isDirectory()) {
                    String subPath = String(dirName) + "/" + entry.name();
                    File subDir = LittleFS.open(subPath.c_str());
                    if (subDir && subDir.isDirectory()) {
                        File subEntry = subDir.openNextFile();
                        while (subEntry) {
                            if (!subEntry.isDirectory()) {
                                const char* sp = subEntry.name();
                                if (sp[0] == '/') sp++;
                                const char* slash = strrchr(sp, '/');
                                if (slash) sp = slash + 1;
                                if (strcmp(sp, "vehicle.json") == 0 ||
                                    (strncmp(sp, "vehicle-", 8) == 0 && String(sp).endsWith(".json"))) {
                                    String res = subPath + "/" + sp;
                                    subEntry.close();
                                    subDir.close();
                                    entry.close();
                                    dir.close();
                                    return res;
                                }
                            }
                            subEntry = subDir.openNextFile();
                        }
                        subDir.close();
                    }
                } else {
                    const char* p = entry.name();
                    if (p[0] == '/') p++;
                    const char* slash = strrchr(p, '/');
                    if (slash) p = slash + 1;
                    if (strcmp(p, "vehicle.json") == 0 ||
                        (strncmp(p, "vehicle-", 8) == 0 && String(p).endsWith(".json"))) {
                        String res = String(dirName) + "/" + p;
                        entry.close();
                        dir.close();
                        return res;
                    }
                }
                entry = dir.openNextFile();
            }
            dir.close();
        }
    }
    return "";
}

bool ConfigParser::loadHardwareConfig(HardwareConfig& config) {
    String path = findHardwareConfig();
    if (path.length() == 0) {
        LOG_CFG_ERR("No hardware config matching /hardware-*.json found on LittleFS");
        return false;
    }
    return loadHardwareConfig(path.c_str(), config);
}

bool ConfigParser::loadVehicleConfig(RcEngineSound::Config& config) {
    String path = findVehicleConfig();
    if (path.length() == 0) {
        LOG_CFG_ERR("No vehicle config matching vehicle-*.json or vehicle.json found on LittleFS");
        return false;
    }
    return loadVehicleConfig(path.c_str(), config);
}

bool ConfigParser::loadHardwareConfig(const char* path, HardwareConfig& config) {
    File file = LittleFS.open(path, "r");
    if (!file) {
        LOG_CFG_ERR("Cannot open: %s", path);
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        LOG_CFG_ERR("JSON error in %s: %s", path, error.c_str());
        return false;
    }

    config = HardwareConfig();

    JsonObjectConst docObj = doc.as<JsonObjectConst>();
    checkUnknownHardwareKeys(docObj);

    const char* nameStr = docObj["name"] | docObj["NAME"] | "";
    if (nameStr[0] != '\0') {
        strlcpy(config.name, nameStr, sizeof(config.name));
    }
    const char* descStr = docObj["description"] | docObj["DESCRIPTION"] | "";
    if (descStr[0] != '\0') {
        strlcpy(config.description, descStr, sizeof(config.description));
    }

    if (!docObj["sound"].isNull() || !docObj["SOUND"].isNull()) {
        JsonObjectConst soundObj = docObj["sound"] | docObj["SOUND"];
        config.sound.volume = soundObj["volume"] | soundObj["VOLUME"] | config.sound.volume;
        config.sound.configured = true;
    }

    if (!docObj["drivetrain"].isNull() || !docObj["DRIVE_MOTOR"].isNull()) {
        JsonObjectConst dtObj = docObj["drivetrain"];
        if (dtObj.isNull()) {
            config.drivetrainType = HardwareConfig::ACKERMANN;
            config.driveMotorCount = 0;
            parseDriveMotor(docObj["DRIVE_MOTOR"], config.driveMotors[0]);
            if (config.driveMotors[0].configured) config.driveMotorCount = 1;
        } else {
            const char* type = dtObj["type"] | dtObj["TYPE"] | "";
            if (strcasecmp(type, "skid_steer") == 0) {
                config.drivetrainType = HardwareConfig::SKID_STEER;
            } else if (strcasecmp(type, "ackermann") == 0) {
                config.drivetrainType = HardwareConfig::ACKERMANN;
            } else {
                if (type[0] != '\0') {
                    LOG_CFG_WARN("drivetrain: unknown type '%s' — inferring from config keys", type);
                }
                config.drivetrainType = (!dtObj["left_motor"].isNull() || !dtObj["LEFT_MOTOR"].isNull())
                                            ? HardwareConfig::SKID_STEER : HardwareConfig::ACKERMANN;
            }

            if (config.drivetrainType == HardwareConfig::SKID_STEER) {
                parseDriveMotor(dtObj["left_motor"] | dtObj["LEFT_MOTOR"], config.leftMotor);
                parseDriveMotor(dtObj["right_motor"] | dtObj["RIGHT_MOTOR"], config.rightMotor);
                config.steeringSensitivity = dtObj["steering_sensitivity"] | dtObj["STEERING_SENSITIVITY"] | config.steeringSensitivity;
                if (!config.leftMotor.configured) {
                    LOG_CFG_WARN("skid-steer: left_motor missing — left track unconfigured");
                }
                if (!config.rightMotor.configured) {
                    LOG_CFG_WARN("skid-steer: right_motor missing — right track unconfigured");
                }
            } else {
                config.driveMotorCount = 0;
                if (dtObj["drive_motors"].is<JsonArrayConst>()) {
                    for (JsonObjectConst motor : dtObj["drive_motors"].as<JsonArrayConst>()) {
                        if (config.driveMotorCount < HardwareConfig::MAX_DRIVE_MOTORS) {
                            parseDriveMotor(motor, config.driveMotors[config.driveMotorCount]);
                            if (config.driveMotors[config.driveMotorCount].configured) {
                                config.driveMotorCount++;
                            }
                        }
                    }
                } else {
                    JsonObjectConst driveObj = dtObj["drive_motor"] | dtObj["DRIVE_MOTOR"];
                    if (!driveObj.isNull()) {
                        parseDriveMotor(driveObj, config.driveMotors[0]);
                        if (config.driveMotors[0].configured) config.driveMotorCount = 1;
                    }
                }

                config.steeringServoCount = 0;
                if (dtObj["steering_servos"].is<JsonArrayConst>()) {
                    for (JsonObjectConst servo : dtObj["steering_servos"].as<JsonArrayConst>()) {
                        if (config.steeringServoCount < HardwareConfig::MAX_STEERING_SERVOS) {
                            parseSteeringServo(servo, config.steeringServos[config.steeringServoCount]);
                            if (config.steeringServos[config.steeringServoCount].configured) {
                                config.steeringServoCount++;
                            }
                        }
                    }
                } else {
                    JsonObjectConst steerObj = dtObj["steering_servo"] | dtObj["STEERING_SERVO"];
                    if (!steerObj.isNull()) {
                        parseSteeringServo(steerObj, config.steeringServos[0]);
                        if (config.steeringServos[0].configured) config.steeringServoCount = 1;
                    }
                }

                if (config.driveMotorCount == 0) {
                    LOG_CFG_WARN("drivetrain: drive_motors missing or unconfigured — vehicle cannot drive!");
                }
            }

            JsonObjectConst autoCenterObj = dtObj["auto_centering"] | dtObj["AUTO_CENTERING"];
            if (!autoCenterObj.isNull()) {
                config.autoCentering.enabled = autoCenterObj["enabled"] | config.autoCentering.enabled;
                config.autoCentering.baseRate = autoCenterObj["base_rate"] | config.autoCentering.baseRate;
                config.autoCentering.speedRate = autoCenterObj["speed_rate"] | config.autoCentering.speedRate;
                config.autoCentering.maxRate = autoCenterObj["max_rate"] | config.autoCentering.maxRate;
                config.autoCentering.holdInReverse = autoCenterObj["hold_in_reverse"] | config.autoCentering.holdInReverse;
                config.autoCentering.configured = true;
            }
        }
    } else {
        LOG_CFG_WARN("drivetrain section missing — drive motor is unconfigured!");
    }

    if (!docObj["steering_servo"].isNull() || !docObj["STEERING_SERVO"].isNull()) {
        if (config.steeringServoCount == 0) {
            parseSteeringServo(docObj["steering_servo"] | docObj["STEERING_SERVO"], config.steeringServos[0]);
            if (config.steeringServos[0].configured) config.steeringServoCount = 1;
        }
    }

    if (!docObj["lights"].isNull() || !docObj["LIGHTS"].isNull()) {
        parseLights(docObj["lights"] | docObj["LIGHTS"], config.lights);
    }

    config.auxMotorCount = 0;
    if (docObj["aux_motors"].is<JsonArrayConst>()) {
        for (JsonObjectConst motor : docObj["aux_motors"].as<JsonArrayConst>()) {
            if (config.auxMotorCount < HardwareConfig::MAX_AUX_MOTORS) {
                parseAuxMotor(motor, config.auxMotors[config.auxMotorCount]);
                if (config.auxMotors[config.auxMotorCount].configured) {
                    config.auxMotorCount++;
                }
            }
        }
    } else if (!docObj["aux_motor"].isNull() || !docObj["AUX_MOTOR"].isNull()) {
        parseAuxMotor(docObj["aux_motor"] | docObj["AUX_MOTOR"], config.auxMotors[0]);
        if (config.auxMotors[0].configured) config.auxMotorCount = 1;
    }

    if (config.drivetrainType == HardwareConfig::SKID_STEER && config.auxMotorCount > 0) {
        LOG_CFG_WARN("aux_motors: ignored in skid-steer mode (the right track owns the second motor output)");
        config.auxMotorCount = 0;
    }

    if (!docObj["aux_light"].isNull() || !docObj["AUX_LIGHT"].isNull()) {
        parseAuxLight(docObj["aux_light"] | docObj["AUX_LIGHT"], config.auxLight);
    }

    if (!docObj["animation"].isNull() || !docObj["ANIMATION"].isNull()) {
        JsonObjectConst animObj = docObj["animation"] | docObj["ANIMATION"];
        config.animation.easingSpeedDegS = animObj["easing_speed_deg_s"] | animObj["EASING_SPEED_DEG_S"] | config.animation.easingSpeedDegS;
        config.animation.easingKIn       = animObj["easing_k_in"]       | animObj["EASING_K_IN"]       | config.animation.easingKIn;
        config.animation.easingKOut      = animObj["easing_k_out"]      | animObj["EASING_K_OUT"]      | config.animation.easingKOut;
        config.animation.fadeDurationMs  = animObj["fade_duration_ms"]  | animObj["FADE_DURATION_MS"]  | config.animation.fadeDurationMs;
        config.animation.configured = true;
    }

    if (!docObj["battery"].isNull() || !docObj["BATTERY"].isNull()) {
        JsonObjectConst batObj = docObj["battery"] | docObj["BATTERY"];
        config.battery.cellCount = batObj["cell_count"] | batObj["CELL_COUNT"] | batObj["cells"] | config.battery.cellCount;
        config.battery.warningVoltage = batObj["warning_voltage"] | batObj["WARNING_VOLTAGE"] | config.battery.warningVoltage;
        config.battery.cutoffVoltage = batObj["cutoff_voltage"] | batObj["CUTOFF_VOLTAGE"] | config.battery.cutoffVoltage;
        config.battery.fullVoltage = batObj["full_voltage"] | batObj["FULL_VOLTAGE"] | config.battery.fullVoltage;

        config.battery.cellCount = constrain(config.battery.cellCount, 0, 4);
        float baseScale = Board::getVoltageDividerRatio();
        if (baseScale <= 0.0f) baseScale = 1.0f;
        float trim = batObj["calibration_factor"] | batObj["CALIBRATION_FACTOR"] | 1.0f;
        if (!batObj["voltage_scale"].isNull() || !batObj["VOLTAGE_SCALE"].isNull()) {
            config.battery.vScale = batObj["voltage_scale"] | batObj["VOLTAGE_SCALE"] | baseScale;
        } else {
            config.battery.vScale = baseScale * trim;
        }
        config.battery.vOffset = batObj["voltage_offset"] | batObj["VOLTAGE_OFFSET"] | 0.0f;
        config.battery.configured = true;
    }

    if (!docObj["power"].isNull() || !docObj["POWER"].isNull()) {
        JsonObjectConst pwrObj = docObj["power"] | docObj["POWER"];
        const char* hw = pwrObj["hardware"] | pwrObj["HARDWARE"] | "";
        config.power.indicatorPin = resolveLightAlias(hw, config);
        config.power.bootLatchS = pwrObj["boot_latch_s"] | pwrObj["BOOT_LATCH_S"] | config.power.bootLatchS;
        config.power.buttonHoldS = pwrObj["button_hold_s"] | pwrObj["BUTTON_HOLD_S"] | config.power.buttonHoldS;
        config.power.disconnectTimeoutS = pwrObj["disconnect_timeout_s"] | pwrObj["DISCONNECT_TIMEOUT_S"] | config.power.disconnectTimeoutS;
        config.power.warningWindowS = pwrObj["warning_window_s"] | pwrObj["WARNING_WINDOW_S"] | config.power.warningWindowS;
        config.power.cutoffDelayS = pwrObj["cutoff_delay_s"] | pwrObj["CUTOFF_DELAY_S"] | config.power.cutoffDelayS;

        config.power.bootLatchS = constrain(config.power.bootLatchS, (uint16_t)0, (uint16_t)30);
        config.power.buttonHoldS = constrain(config.power.buttonHoldS, (uint16_t)1, (uint16_t)30);
        config.power.disconnectTimeoutS = constrain(config.power.disconnectTimeoutS, (uint16_t)0, (uint16_t)3600);
        config.power.warningWindowS = constrain(config.power.warningWindowS, (uint16_t)1, (uint16_t)60);
        config.power.cutoffDelayS = constrain(config.power.cutoffDelayS, (uint16_t)0, (uint16_t)60);
        config.power.configured = true;
    }

    if (!docObj["charging"].isNull() || !docObj["CHARGING"].isNull()) {
        JsonObjectConst chgObj = docObj["charging"] | docObj["CHARGING"];
        const char* hw = chgObj["hardware"] | chgObj["HARDWARE"] | "";
        config.charging.pin = resolveLightAlias(hw, config);
        const char* modeStr = chgObj["mode"] | chgObj["MODE"] | "solid";
        if (strcasecmp(modeStr, "blink") == 0) config.charging.mode = 1;
        else if (strcasecmp(modeStr, "pulse") == 0) config.charging.mode = 2;
        else config.charging.mode = 0;
        config.charging.configured = (config.charging.pin != 0xFF);
    }

    validateHardwareConfig(config);

    Serial.printf("[ConfigParser] Loaded hardware config: %s\n", path);
    return true;
}

bool ConfigParser::loadVehicleConfig(const char* path, RcEngineSound::Config& config) {
    File file = LittleFS.open(path, "r");
    if (!file) {
        LOG_CFG_ERR("Cannot open: %s", path);
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        LOG_CFG_ERR("JSON error in %s: %s", path, error.c_str());
        return false;
    }

    config = RcEngineSound::Config();

    JsonObjectConst vehObj = doc["vehicle"] | doc["VEHICLE"];
    strlcpy(config.name, vehObj["name"] | vehObj["NAME"] | "Unknown", sizeof(config.name));
    strlcpy(config.description, vehObj["description"] | vehObj["DESCRIPTION"] | "", sizeof(config.description));
    strlcpy(config.soundSet, vehObj["sound_set"] | vehObj["SOUND_SET"] | config.name, sizeof(config.soundSet));
    const char* vehicleTypeStr = vehObj["type"] | vehObj["TYPE"] | "TRUCK";
    if (strcasecmp(vehicleTypeStr, "LOCOMOTIVE") == 0) {
        config.type = RcEngineSound::VEHICLE_LOCOMOTIVE;
    } else if (strcasecmp(vehicleTypeStr, "EXCAVATOR") == 0) {
        config.type = RcEngineSound::VEHICLE_EXCAVATOR;
    } else if (strcasecmp(vehicleTypeStr, "TRUCK") == 0) {
        config.type = RcEngineSound::VEHICLE_TRUCK;
    } else {
        config.type = RcEngineSound::VEHICLE_UNKNOWN;
        LOG_CFG_WARN("unknown vehicle type '%s' — defaulting to truck", vehicleTypeStr);
    }

    parseEngine(doc, config);
    parseSoundVolumes(doc, config);
    parseTransmission(doc, config);
    parseFeatures(doc, config);
    parseLoopPoints(doc, config);
    parseMixWeights(doc, config);

    checkUnknownVehicleKeys(doc.as<JsonObjectConst>());
    validateVehicleConfig(config);

    Serial.printf("[ConfigParser] Loaded vehicle config: %s (sound_set=%s, type=%s)\n",
                  config.name, config.soundSet, vehicleTypeStr);
    return true;
}

bool ConfigParser::loadSounds(const RcEngineSound::Config& cfg, SoundData& soundData) {
    soundData = SoundData();

    String soundSet = cfg.soundSet[0] != '\0' ? cfg.soundSet : cfg.name;
    soundSet.replace(" ", "");

    const char* typeStr = "truck";
    if (cfg.type == RcEngineSound::VEHICLE_LOCOMOTIVE) {
        typeStr = "locomotive";
    } else if (cfg.type == RcEngineSound::VEHICLE_EXCAVATOR) {
        typeStr = "excavator";
    }

    for (int i = 0; i < SOUND_COUNT; i++) {
        const char* slot = soundTypeNames[i];

        String paths[] = {
            "/vehicle_configs/" + soundSet + "/sounds/" + String(slot) + ".pcm",
            "/vehicle_config/" + soundSet + "/sounds/" + String(slot) + ".pcm",
            "/sounds/vehicles/" + soundSet + "/" + String(slot) + ".pcm",
            "/vehicle_configs/common/" + String(typeStr) + "/" + String(slot) + ".pcm",
            "/vehicle_config/common/" + String(typeStr) + "/" + String(slot) + ".pcm",
            "/sounds/common/" + String(typeStr) + "/" + String(slot) + ".pcm",
            "/sounds/presets/" + String(typeStr) + "/" + String(slot) + ".pcm",
            "/sounds/generic/" + String(slot) + ".pcm",
            "/sounds/" + String(slot) + ".pcm"
        };

        for (const String& p : paths) {
            soundData.slots[i] = loadSoundSlot(p.c_str());
            if (soundData.slots[i].samples) {
                break;
            }
        }
    }

    int loaded = 0;
    for (int i = 0; i < SOUND_COUNT; i++) {
        if (soundData.slots[i].samples) loaded++;
    }
    if (loaded > 0) soundData.isDynamic = true;
    Serial.printf("[ConfigParser] Loaded %d sounds for %s (sound_set=%s, type=%s)\n",
                  loaded, cfg.name, soundSet.c_str(), typeStr);
    return loaded > 0;
}

bool ConfigParser::loadSounds(const char* vehicleName, SoundData& soundData) {
    RcEngineSound::Config cfg;
    strlcpy(cfg.name, vehicleName, sizeof(cfg.name));
    strlcpy(cfg.soundSet, vehicleName, sizeof(cfg.soundSet));
    return loadSounds(cfg, soundData);
}

void ConfigParser::printFileTree(const char* dirPath, int depth) {
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

void ConfigParser::printFilesystemInfo() {
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

SoundSlot ConfigParser::loadSoundSlot(const char* path) {
    File file = LittleFS.open(path, "r");
    if (!file) return SoundSlot();

    PcmHeader hdr;
    if (file.read((uint8_t*)&hdr, sizeof(hdr)) != sizeof(hdr)) {
        file.close();
        return SoundSlot();
    }

    if (hdr.magic[0] != 'R' || hdr.magic[1] != 'P') {
        LOG_CFG_WARN("%s: invalid PCM magic '%c%c' (expected 'RP')", path, hdr.magic[0], hdr.magic[1]);
        file.close();
        return SoundSlot();
    }

    if (hdr.sampleCount == 0) {
        file.close();
        return SoundSlot();
    }

    int8_t* buffer = (int8_t*)ps_malloc(hdr.sampleCount);
    if (!buffer) {
        LOG_CFG_ERR("%s: ps_malloc(%u) failed", path, hdr.sampleCount);
        file.close();
        return SoundSlot();
    }

    size_t bytesRead = file.read((uint8_t*)buffer, hdr.sampleCount);
    file.close();

    if (bytesRead != hdr.sampleCount) {
        LOG_CFG_WARN("%s: expected %u samples, read %u bytes", path, hdr.sampleCount, (uint32_t)bytesRead);
    }

    SoundSlot slot;
    slot.samples = buffer;
    slot.sampleCount = (uint32_t)bytesRead;
    slot.sampleRate = hdr.sampleRate;
    return slot;
}

// ── Validation / key-check helpers ──────────────────────────────────────────

bool ConfigParser::keyAllowed(const char* key, const char* const* allowed, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (strcasecmp(key, allowed[i]) == 0) return true;
    }
    return false;
}

void ConfigParser::checkKeys(JsonObjectConst obj, const char* section,
                      const char* const* allowed, size_t n) {
    if (obj.isNull()) return;
    for (JsonPairConst kv : obj) {
        const char* key = kv.key().c_str();
        if (!keyAllowed(key, allowed, n)) {
            LOG_CFG_WARN("%s: unknown key '%s' (ignored)", section, key);
        }
    }
}

void ConfigParser::warnUnresolvedHardware(const char* where, const char* hw) {
    if (hw[0] != '\0' && PinMapper::resolve(hw) == 0xFF) {
        LOG_CFG_WARN("%s: hardware '%s' not recognized — output not configured", where, hw);
    }
}

void ConfigParser::checkUnknownHardwareKeys(JsonObjectConst doc) {
    static const char* top[] = {"name", "description", "sound", "drivetrain",
                                "steering_servo", "lights", "animation",
                                "battery", "power", "charging",
                                "drive_motor", "aux_motors", "aux_motor", "aux_light"};
    checkKeys(doc, "hardware", top, 14);

    JsonObjectConst sound = doc["sound"];
    if (!sound.isNull()) {
        static const char* k[] = {"volume"};
        checkKeys(sound, "sound", k, 1);
    }

    JsonObjectConst dt = doc["drivetrain"];
    if (!dt.isNull()) {
        static const char* k[] = {"drive_motors", "drive_motor", "steering_servos",
                                  "steering_servo", "left_motor", "right_motor",
                                  "steering_sensitivity", "type", "auto_centering"};
        checkKeys(dt, "drivetrain", k, 9);
        JsonObjectConst ac = dt["auto_centering"];
        if (!ac.isNull()) {
            static const char* ack[] = {"enabled", "base_rate", "speed_rate", "max_rate", "hold_in_reverse"};
            checkKeys(ac, "auto_centering", ack, 5);
        }
        if (dt["drive_motors"].is<JsonArrayConst>()) {
            for (JsonObjectConst m : dt["drive_motors"].as<JsonArrayConst>()) {
                static const char* mk[] = {"hardware", "frequency", "direction", "duty"};
                checkKeys(m, "drive_motors[]", mk, 4);
                JsonObjectConst duty = m["duty"];
                if (!duty.isNull()) {
                    static const char* dk[] = {"min", "max"};
                    checkKeys(duty, "duty", dk, 2);
                }
            }
        }
        if (dt["steering_servos"].is<JsonArrayConst>()) {
            for (JsonObjectConst ss : dt["steering_servos"].as<JsonArrayConst>()) {
                static const char* sk[] = {"hardware", "frequency", "endpoints"};
                checkKeys(ss, "steering_servos[]", sk, 3);
                JsonObjectConst ep = ss["endpoints"];
                if (!ep.isNull()) {
                    static const char* ek[] = {"left", "right", "center"};
                    checkKeys(ep, "endpoints", ek, 3);
                }
            }
        }
        for (const char* motorKey : {"drive_motor", "left_motor", "right_motor"}) {
            JsonObjectConst m = dt[motorKey];
            if (m.isNull()) continue;
            static const char* mk[] = {"hardware", "frequency", "direction", "duty"};
            checkKeys(m, motorKey, mk, 4);
            JsonObjectConst duty = m["duty"];
            if (!duty.isNull()) {
                static const char* dk[] = {"min", "max"};
                checkKeys(duty, "duty", dk, 2);
            }
        }
        JsonObjectConst ss = dt["steering_servo"];
        if (!ss.isNull()) {
            static const char* sk[] = {"hardware", "frequency", "endpoints"};
            checkKeys(ss, "steering_servo", sk, 3);
            JsonObjectConst ep = ss["endpoints"];
            if (!ep.isNull()) {
                static const char* ek[] = {"left", "right", "center"};
                checkKeys(ep, "endpoints", ek, 3);
            }
        }
    }

    JsonObjectConst lights = doc["lights"];
    if (!lights.isNull()) {
        static const char* lk[] = {"head_light", "full_beam", "high_beam",
                                   "fog_lamp", "fog_light", "tail_light",
                                   "brake_light", "turn_light", "reversing_light",
                                   "beacon", "cab_light", "work_light", "work_lamp",
                                   "step_light", "aux_light", "ditch_light"};
        checkKeys(lights, "lights", lk, 16);
        for (const char* lightKey : {"head_light", "full_beam", "high_beam",
                                     "fog_lamp", "fog_light",
                                     "tail_light", "brake_light",
                                     "reversing_light", "beacon",
                                     "cab_light", "work_light", "work_lamp",
                                     "step_light", "aux_light"}) {
            JsonObjectConst l = lights[lightKey];
            if (l.isNull()) continue;
            static const char* hk[] = {"hardware", "brightness_max"};
            checkKeys(l, lightKey, hk, 2);
        }
        JsonObjectConst turn = lights["turn_light"];
        if (!turn.isNull()) {
            static const char* tk[] = {"left", "right", "brightness_max",
                                       "type", "interval_on", "interval_off",
                                       "left_hardware", "right_hardware"};
            checkKeys(turn, "turn_light", tk, 8);
            for (const char* side : {"left", "right"}) {
                JsonObjectConst s = turn[side];
                if (s.isNull()) continue;
                static const char* sk[] = {"hardware"};
                checkKeys(s, side, sk, 1);
            }
        }
        JsonObjectConst ditch = lights["ditch_light"];
        if (!ditch.isNull()) {
            static const char* dk[] = {"left", "right", "brightness_max",
                                       "interval_ms",
                                       "left_hardware", "right_hardware"};
            checkKeys(ditch, "ditch_light", dk, 6);
            for (const char* side : {"left", "right"}) {
                JsonObjectConst s = ditch[side];
                if (s.isNull()) continue;
                static const char* sk[] = {"hardware"};
                checkKeys(s, side, sk, 1);
            }
        }
    }

    if (doc["aux_motors"].is<JsonArrayConst>()) {
        for (JsonObjectConst am : doc["aux_motors"].as<JsonArrayConst>()) {
            static const char* k[] = {"hardware", "frequency", "direction", "duty", "type"};
            checkKeys(am, "aux_motors[]", k, 5);
            JsonObjectConst duty = am["duty"];
            if (!duty.isNull()) {
                static const char* dk[] = {"min", "max"};
                checkKeys(duty, "aux_motors[].duty", dk, 2);
            }
        }
    }
    JsonObjectConst auxMotor = doc["aux_motor"];
    if (!auxMotor.isNull()) {
        static const char* k[] = {"hardware", "frequency",
                                 "direction", "duty", "type"};
        checkKeys(auxMotor, "aux_motor", k, 5);
        JsonObjectConst duty = auxMotor["duty"];
        if (!duty.isNull()) {
            static const char* dk[] = {"min", "max"};
            checkKeys(duty, "aux_motor.duty", dk, 2);
        }
    }
    JsonObjectConst auxLight = doc["aux_light"];
    if (!auxLight.isNull()) {
        static const char* k[] = {"hardware", "brightness_max"};
        checkKeys(auxLight, "aux_light", k, 2);
    }

    JsonObjectConst anim = doc["animation"];
    if (!anim.isNull()) {
        static const char* ak[] = {"easing_speed_deg_s", "easing_k_in",
                                   "easing_k_out", "fade_duration_ms"};
        checkKeys(anim, "animation", ak, 4);
    }

    JsonObjectConst bat = doc["battery"];
    if (!bat.isNull()) {
        static const char* bk[] = {"cell_count", "warning_voltage", "cutoff_voltage",
                                   "full_voltage", "cells",
                                   "voltage_scale", "voltage_offset"};
        checkKeys(bat, "battery", bk, 7);
    }

    JsonObjectConst pwr = doc["power"];
    if (!pwr.isNull()) {
        static const char* pk[] = {"hardware", "boot_latch_s", "button_hold_s",
                                   "disconnect_timeout_s", "warning_window_s",
                                   "cutoff_delay_s"};
        checkKeys(pwr, "power", pk, 6);
    }

    JsonObjectConst chg = doc["charging"];
    if (!chg.isNull()) {
        static const char* ck[] = {"hardware", "mode"};
        checkKeys(chg, "charging", ck, 2);
    }
}

void ConfigParser::validateHardwareConfig(const HardwareConfig& cfg) {
    auto checkMotor = [&](const HardwareConfig::DriveMotor& m, const char* name) {
        if (m.type == HardwareConfig::DriveMotor::NONE) return;
        if (m.duty.min > m.duty.max) {
            LOG_CFG_WARN("%s: duty.min (%d) > duty.max (%d)",
                         name, m.duty.min, m.duty.max);
        }
        if (m.duty.min > 100 || m.duty.max > 100) {
            LOG_CFG_WARN("%s: duty out of range (min=%d max=%d, expected 0-100)",
                         name, m.duty.min, m.duty.max);
        }
        if (m.frequency == 0 || m.frequency > 100000) {
            LOG_CFG_WARN("%s: frequency %d Hz out of sane range",
                         name, m.frequency);
        }
    };

    if (cfg.drivetrainType == HardwareConfig::SKID_STEER) {
        checkMotor(cfg.leftMotor, "left_motor");
        checkMotor(cfg.rightMotor, "right_motor");
        if (cfg.leftMotor.type == HardwareConfig::DriveMotor::NONE) {
            LOG_CFG_WARN("skid-steer: left_motor missing — left track not configured");
        }
        if (cfg.rightMotor.type == HardwareConfig::DriveMotor::NONE) {
            LOG_CFG_WARN("skid-steer: right_motor missing — right track not configured");
        }
    } else {
        for (uint8_t i = 0; i < cfg.driveMotorCount; i++) {
            checkMotor(cfg.driveMotors[i], "drive_motors");
        }
        if (cfg.driveMotorCount == 0) {
            LOG_CFG_WARN("drivetrain: drive_motors missing or unconfigured — vehicle cannot drive!");
        }
    }

    for (uint8_t i = 0; i < cfg.auxMotorCount; i++) {
        checkMotor(cfg.auxMotors[i].motor, "aux_motors");
    }

    for (uint8_t i = 0; i < cfg.steeringServoCount; i++) {
        const auto& s = cfg.steeringServos[i];
        if (s.hardwareId != 0xFF) {
            if (s.frequency < 40 || s.frequency > 400) {
                LOG_CFG_WARN("steering_servos: frequency %d Hz outside 40-400", s.frequency);
            }
            const auto& ep = s.endpoints;
            if (ep.left < 500 || ep.left > 2500 || ep.right < 500 || ep.right > 2500 ||
                ep.center < 500 || ep.center > 2500) {
                LOG_CFG_WARN("steering_servos: endpoints out of range (L=%d R=%d C=%d, expected ~500-2500 us)",
                             ep.left, ep.right, ep.center);
            }
        }
    }

    auto checkLight = [&](bool configured, uint8_t brightness, const char* name) {
        if (configured && brightness > 100) {
            LOG_CFG_WARN("%s: brightness %d > 100", name, brightness);
        }
    };
    checkLight(cfg.lights.headLight.configured, cfg.lights.headLight.brightness, "head_light");
    checkLight(cfg.lights.tailLight.configured, cfg.lights.tailLight.brightness, "tail_light");
    checkLight(cfg.lights.turnLight.configured, cfg.lights.turnLight.brightness, "turn_light");
    checkLight(cfg.lights.ditchLight.configured, cfg.lights.ditchLight.brightness, "ditch_light");

    if (cfg.autoCentering.configured) {
        if (cfg.autoCentering.baseRate < 0.0f || cfg.autoCentering.speedRate < 0.0f || cfg.autoCentering.maxRate < 0.0f) {
            LOG_CFG_WARN("auto_centering: rates must be non-negative");
        }
    }
}

// ── Hardware-config parsers ─────────────────────────────────────────────────

void ConfigParser::parseDriveMotor(JsonVariantConst motor, HardwareConfig::DriveMotor& config) {
    if (motor.isNull()) return;
    const char* hw = motor["hardware"] | motor["HARDWARE"] | "";
    warnUnresolvedHardware("drive motor", hw);
    if (strcmp(hw, "DRIVER_A") == 0 || strcmp(hw, "DRIVER_B") == 0) {
        config.type = HardwareConfig::DriveMotor::DRIVER;
    } else if (hw[0] == 'S') {
        config.type = HardwareConfig::DriveMotor::ESC;
    }
    config.hardwareId = PinMapper::resolve(hw);
    config.frequency = motor["frequency"] | motor["FREQUENCY"] | config.frequency;

    const char* dir = motor["direction"] | motor["DIRECTION"] | "FORWARD";
    if (strcasecmp(dir, "REVERSE") == 0) config.direction = HardwareConfig::DriveMotor::REVERSE;
    else if (strcasecmp(dir, "UNI_FORWARD") == 0) config.direction = HardwareConfig::DriveMotor::UNI_FORWARD;
    else if (strcasecmp(dir, "UNI_REVERSE") == 0) config.direction = HardwareConfig::DriveMotor::UNI_REVERSE;
    else {
        if (strcasecmp(dir, "FORWARD") != 0) {
            LOG_CFG_WARN("drive motor: unknown direction '%s' — defaulting to forward", dir);
        }
        config.direction = HardwareConfig::DriveMotor::FORWARD;
    }

    JsonVariantConst dutyObj = motor["duty"] | motor["DUTY"];
    config.duty.min = dutyObj["min"] | dutyObj["MIN"] | config.duty.min;
    config.duty.max = dutyObj["max"] | dutyObj["MAX"] | config.duty.max;
    config.configured = (config.hardwareId != 0xFF);
}

void ConfigParser::parseSteeringServo(JsonVariantConst servo, HardwareConfig::SteeringServo& config) {
    if (servo.isNull()) return;
    const char* hw = servo["hardware"] | servo["HARDWARE"] | "";
    warnUnresolvedHardware("steering_servo", hw);
    config.hardwareId = PinMapper::resolve(hw);
    config.frequency = servo["frequency"] | servo["FREQUENCY"] | config.frequency;

    JsonVariantConst epObj = servo["endpoints"] | servo["ENDPOINTS"];
    config.endpoints.left = epObj["left"] | epObj["LEFT"] | config.endpoints.left;
    config.endpoints.right = epObj["right"] | epObj["RIGHT"] | config.endpoints.right;
    config.endpoints.center = epObj["center"] | epObj["CENTER"] | config.endpoints.center;
    config.configured = (config.hardwareId != 0xFF);
}

uint8_t ConfigParser::resolveLightAlias(const char* hw, const HardwareConfig& config) {
    if (hw == nullptr || hw[0] == '\0') return 0xFF;
    uint8_t pin = PinMapper::resolve(hw);
    if (pin != 0xFF) return pin;
    if (strcasecmp(hw, "head_light") == 0 && config.lights.headLight.configured) return config.lights.headLight.pin;
    if ((strcasecmp(hw, "full_beam") == 0 || strcasecmp(hw, "high_beam") == 0) && config.lights.fullBeam.configured) return config.lights.fullBeam.pin;
    if ((strcasecmp(hw, "fog_lamp") == 0 || strcasecmp(hw, "fog_light") == 0) && config.lights.fogLamp.configured) return config.lights.fogLamp.pin;
    if (strcasecmp(hw, "tail_light") == 0 && config.lights.tailLight.configured) return config.lights.tailLight.pin;
    if (strcasecmp(hw, "brake_light") == 0 && config.lights.brakeLight.configured) return config.lights.brakeLight.pin;
    if (strcasecmp(hw, "cab_light") == 0 && config.lights.cabLight.configured) return config.lights.cabLight.pin;
    if (strcasecmp(hw, "step_light") == 0 && config.lights.stepLight.configured) return config.lights.stepLight.pin;
    if (strcasecmp(hw, "beacon") == 0 && config.lights.beacon.configured) return config.lights.beacon.pin;
    if ((strcasecmp(hw, "work_light") == 0 || strcasecmp(hw, "work_lamp") == 0) && config.lights.workLight.configured) return config.lights.workLight.pin;
    if (strcasecmp(hw, "aux_light") == 0 && (config.lights.auxLight.configured || config.auxLight.configured)) return config.lights.auxLight.configured ? config.lights.auxLight.pin : config.auxLight.pin;
    if (strcasecmp(hw, "turn_light") == 0 && config.lights.turnLight.configured) return config.lights.turnLight.leftPin != 0xFF ? config.lights.turnLight.leftPin : config.lights.turnLight.rightPin;
    return 0xFF;
}

void ConfigParser::parseLight(JsonVariantConst light, HardwareConfig::Lights::Light& config, const char* name, uint8_t defaultBrightness) {
    if (light.isNull()) return;
    config.pinCount = 0;
    if (light["hardware"].is<JsonArrayConst>()) {
        for (JsonVariantConst p : light["hardware"].as<JsonArrayConst>()) {
            const char* hw = p.as<const char*>();
            if (hw && config.pinCount < HardwareConfig::MAX_PINS_PER_LIGHT) {
                warnUnresolvedHardware(name, hw);
                uint8_t pin = PinMapper::resolve(hw);
                if (pin != 0xFF) config.pins[config.pinCount++] = pin;
            }
        }
    } else {
        const char* hw = light["hardware"] | "";
        if (hw[0] != '\0') {
            warnUnresolvedHardware(name, hw);
            uint8_t pin = PinMapper::resolve(hw);
            if (pin != 0xFF && config.pinCount < HardwareConfig::MAX_PINS_PER_LIGHT) {
                config.pins[config.pinCount++] = pin;
            }
        }
    }
    config.brightness = light["brightness_max"] | defaultBrightness;
    config.pin = (config.pinCount > 0) ? config.pins[0] : 0xFF;
    config.configured = (config.pinCount > 0);
}

void ConfigParser::parseLights(JsonObjectConst lights, HardwareConfig::Lights& config) {
    if (lights.isNull()) return;

    parseLight(lights["head_light"] | lights["HEAD_LIGHT"], config.headLight, "head_light", 60);
    parseLight(lights["full_beam"] | lights["FULL_BEAM"] | lights["high_beam"] | lights["HIGH_BEAM"], config.fullBeam, "full_beam", 100);
    parseLight(lights["fog_lamp"] | lights["FOG_LAMP"] | lights["fog_light"] | lights["FOG_LIGHT"], config.fogLamp, "fog_lamp", 60);
    parseLight(lights["tail_light"] | lights["TAIL_LIGHT"], config.tailLight, "tail_light", 60);
    parseLight(lights["brake_light"] | lights["BRAKE_LIGHT"], config.brakeLight, "brake_light", 100);

    JsonVariantConst ditch = lights["ditch_light"] | lights["DITCH_LIGHT"];
    if (!ditch.isNull()) {
        JsonVariantConst ditchL = ditch["left"] | ditch["LEFT"];
        JsonVariantConst ditchR = ditch["right"] | ditch["RIGHT"];
        const char* hwL = ditchL["hardware"] | ditchL["HARDWARE"] | ditch["left_hardware"] | "";
        const char* hwR = ditchR["hardware"] | ditchR["HARDWARE"] | ditch["right_hardware"] | "";
        warnUnresolvedHardware("ditch_light.left", hwL);
        warnUnresolvedHardware("ditch_light.right", hwR);
        config.ditchLight.leftPin = PinMapper::resolve(hwL);
        config.ditchLight.rightPin = PinMapper::resolve(hwR);
        config.ditchLight.brightness = ditch["brightness_max"] | ditch["BRIGHTNESS_MAX"] | 100;
        config.ditchLight.intervalMs = ditch["interval_ms"] | ditch["INTERVAL_MS"] | 8;
        config.ditchLight.configured = config.ditchLight.leftPin != 0xFF || config.ditchLight.rightPin != 0xFF;
    }

    parseLight(lights["step_light"] | lights["STEP_LIGHT"], config.stepLight, "step_light", 30);
    parseLight(lights["cab_light"] | lights["CAB_LIGHT"], config.cabLight, "cab_light", 40);

    JsonVariantConst turn = lights["turn_light"] | lights["TURN_LIGHT"];
    if (!turn.isNull()) {
        JsonVariantConst turnL = turn["left"] | turn["LEFT"];
        JsonVariantConst turnR = turn["right"] | turn["RIGHT"];
        const char* hwL = turnL["hardware"] | turnL["HARDWARE"] | turn["left_hardware"] | "";
        const char* hwR = turnR["hardware"] | turnR["HARDWARE"] | turn["right_hardware"] | "";
        warnUnresolvedHardware("turn_light.left", hwL);
        warnUnresolvedHardware("turn_light.right", hwR);
        config.turnLight.leftPin = PinMapper::resolve(hwL);
        config.turnLight.rightPin = PinMapper::resolve(hwR);
        config.turnLight.brightness = turn["brightness_max"] | turn["BRIGHTNESS_MAX"] | 60;
        config.turnLight.intervalOn = turn["interval_on"] | turn["INTERVAL_ON"] | 300;
        config.turnLight.intervalOff = turn["interval_off"] | turn["INTERVAL_OFF"] | 300;
        config.turnLight.configured = config.turnLight.leftPin != 0xFF || config.turnLight.rightPin != 0xFF;
    }

    JsonVariantConst reversing = lights["reversing_light"] | lights["REVERSING_LIGHT"];
    if (!reversing.isNull()) {
        config.reversingLight.pinCount = 0;
        if (reversing["hardware"].is<JsonArrayConst>()) {
            for (JsonVariantConst p : reversing["hardware"].as<JsonArrayConst>()) {
                const char* hw = p.as<const char*>();
                if (hw && config.reversingLight.pinCount < HardwareConfig::MAX_PINS_PER_LIGHT) {
                    uint8_t pin = PinMapper::resolve(hw);
                    if (pin != 0xFF) config.reversingLight.pins[config.reversingLight.pinCount++] = pin;
                }
            }
        } else {
            const char* hw = reversing["hardware"] | reversing["HARDWARE"] | "";
            uint8_t pin = PinMapper::resolve(hw);
            if (pin == 0xFF) {
                if ((strcasecmp(hw, "head_light") == 0 || strcasecmp(hw, "HEAD_LIGHT") == 0) && config.headLight.configured) pin = config.headLight.pin;
                else if ((strcasecmp(hw, "tail_light") == 0 || strcasecmp(hw, "TAIL_LIGHT") == 0) && config.tailLight.configured) pin = config.tailLight.pin;
                else if ((strcasecmp(hw, "brake_light") == 0 || strcasecmp(hw, "BRAKE_LIGHT") == 0) && config.brakeLight.configured) pin = config.brakeLight.pin;
            }
            if (pin != 0xFF) {
                config.reversingLight.pins[config.reversingLight.pinCount++] = pin;
            } else if (hw[0] != '\0') {
                LOG_CFG_WARN("reversing_light: hardware '%s' not recognized — output not configured", hw);
            }
        }
        config.reversingLight.brightness = 100;
        config.reversingLight.pin = (config.reversingLight.pinCount > 0) ? config.reversingLight.pins[0] : 0xFF;
        config.reversingLight.configured = (config.reversingLight.pinCount > 0);
    }

    parseLight(lights["beacon"] | lights["BEACON"], config.beacon, "beacon", 100);
    parseLight(lights["work_light"] | lights["WORK_LIGHT"] | lights["work_lamp"] | lights["WORK_LAMP"], config.workLight, "work_light", 80);
    parseLight(lights["aux_light"] | lights["AUX_LIGHT"], config.auxLight, "aux_light", 60);
}

void ConfigParser::parseAuxMotor(JsonVariantConst motor, HardwareConfig::AuxMotor& config) {
    if (motor.isNull()) return;
    const char* hw = motor["hardware"] | motor["HARDWARE"] | "";
    warnUnresolvedHardware("aux_motor", hw);
    if (strcmp(hw, "DRIVER_A") == 0 || strcmp(hw, "DRIVER_B") == 0) {
        config.motor.type = HardwareConfig::DriveMotor::DRIVER;
    } else if (hw[0] == 'S') {
        config.motor.type = HardwareConfig::DriveMotor::ESC;
    }
    config.motor.hardwareId = PinMapper::resolve(hw);
    config.motor.frequency = motor["frequency"] | motor["FREQUENCY"] | 20000;

    const char* dir = motor["direction"] | motor["DIRECTION"] | "FORWARD";
    if (strcasecmp(dir, "REVERSE") == 0) config.motor.direction = HardwareConfig::DriveMotor::REVERSE;
    else if (strcasecmp(dir, "UNI_FORWARD") == 0) config.motor.direction = HardwareConfig::DriveMotor::UNI_FORWARD;
    else if (strcasecmp(dir, "UNI_REVERSE") == 0) config.motor.direction = HardwareConfig::DriveMotor::UNI_REVERSE;
    else {
        if (strcasecmp(dir, "FORWARD") != 0) {
            LOG_CFG_WARN("aux_motor: unknown direction '%s' — defaulting to forward", dir);
        }
        config.motor.direction = HardwareConfig::DriveMotor::FORWARD;
    }

    JsonVariantConst dutyObj = motor["duty"] | motor["DUTY"];
    config.motor.duty.min = dutyObj["min"] | dutyObj["MIN"] | 20;
    config.motor.duty.max = dutyObj["max"] | dutyObj["MAX"] | 90;

    const char* type = motor["type"] | motor["TYPE"] | "mixer";
    if (strcasecmp(type, "mixer") == 0) {
        config.purpose = HardwareConfig::AuxMotor::MIXER;
    } else if (strcasecmp(type, "tipper") == 0) {
        config.purpose = HardwareConfig::AuxMotor::TIPPER;
    } else if (strcasecmp(type, "trailer_dcc") == 0) {
        config.purpose = HardwareConfig::AuxMotor::TRAILER_DCC;
        config.motor.type = HardwareConfig::DriveMotor::NONE;
        LOG_CFG_WARN("aux_motor: type 'trailer_dcc' not yet implemented — aux channel not configured");
    } else {
        LOG_CFG_WARN("aux_motor: unknown type '%s' — defaulting to mixer", type);
        config.purpose = HardwareConfig::AuxMotor::MIXER;
    }
    config.configured = (config.motor.hardwareId != 0xFF && config.purpose != HardwareConfig::AuxMotor::NONE);
}

void ConfigParser::parseAuxLight(JsonVariantConst light, HardwareConfig::AuxLight& config) {
    if (light.isNull()) return;
    config.pinCount = 0;
    if (light["hardware"].is<JsonArrayConst>()) {
        for (JsonVariantConst p : light["hardware"].as<JsonArrayConst>()) {
            const char* hw = p.as<const char*>();
            if (hw && config.pinCount < HardwareConfig::MAX_PINS_PER_LIGHT) {
                warnUnresolvedHardware("aux_light", hw);
                uint8_t pin = PinMapper::resolve(hw);
                if (pin != 0xFF) config.pins[config.pinCount++] = pin;
            }
        }
    } else {
        const char* hw = light["hardware"] | light["HARDWARE"] | "";
        if (hw[0] != '\0') {
            warnUnresolvedHardware("aux_light", hw);
            uint8_t pin = PinMapper::resolve(hw);
            if (pin != 0xFF && config.pinCount < HardwareConfig::MAX_PINS_PER_LIGHT) {
                config.pins[config.pinCount++] = pin;
            }
        }
    }
    config.brightness = light["brightness_max"] | light["BRIGHTNESS_MAX"] | 60;
    config.pin = (config.pinCount > 0) ? config.pins[0] : 0xFF;
    config.configured = (config.pinCount > 0);
}

// ── Vehicle-config parsers / validators ─────────────────────────────────────

void ConfigParser::checkUnknownVehicleKeys(JsonObjectConst doc) {
    static const char* top[] = {"vehicle", "engine", "sound_volumes",
                                "transmission", "features", "loop_points",
                                "mix_weights"};
    checkKeys(doc, "vehicle", top, 7);

    JsonObjectConst veh = doc["vehicle"] | doc["VEHICLE"];
    if (!veh.isNull()) {
        static const char* k[] = {"name", "description", "sound_set", "type"};
        checkKeys(veh, "vehicle", k, 4);
    }
}

void ConfigParser::parseEngine(JsonDocument& doc, RcEngineSound::Config& cfg) {
    JsonVariantConst eng = doc["engine"] | doc["ENGINE"];
    if (eng.isNull()) {
        cfg.engine.hasEngine = false;
        return;
    }
    cfg.engine.hasEngine = true;
    cfg.engine.acc = eng["acceleration"] | eng["ACCELERATION"] | 2;
    cfg.engine.dec = eng["deceleration"] | eng["DECELERATION"] | 2;
    cfg.engine.brakeDec = eng["brake_deceleration"] | eng["BRAKE_DECELERATION"] | 10;
    cfg.engine.inertia = eng["inertia"] | eng["INERTIA"] | 10;
    cfg.engine.escRampTime = eng["esc_ramp_time"] | eng["ESC_RAMP_TIME"] | 20;
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

    uint16_t defaultStop = (cfg.type == RcEngineSound::VEHICLE_LOCOMOTIVE) ? 2800 :
                           (cfg.type == RcEngineSound::VEHICLE_EXCAVATOR)  ? 1800 : 1400;
    cfg.engine.stopDuration = eng["stop_duration"] | eng["STOP_DURATION"] |
                              eng["stop_duration_ms"] | eng["STOP_DURATION_MS"] | defaultStop;
}

void ConfigParser::parseSoundVolumes(JsonDocument& doc, RcEngineSound::Config& cfg) {
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
    cfg.sound.bell = sv["bell"] | sv["BELL"] | (cfg.type == RcEngineSound::VEHICLE_LOCOMOTIVE ? 100 : 0);
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

void ConfigParser::parseTransmission(JsonDocument& doc, RcEngineSound::Config& cfg) {
    JsonVariantConst tr = doc["transmission"] | doc["TRANSMISSION"];
    const char* transTypeStr = tr["type"] | tr["TYPE"] | "NONE";
    if (strcasecmp(transTypeStr, "AUTOMATIC") == 0) cfg.transmission.type = RcEngineSound::TRANS_AUTOMATIC;
    else if (strcasecmp(transTypeStr, "MANUAL") == 0) cfg.transmission.type = RcEngineSound::TRANS_MANUAL;
    else {
        if (strcasecmp(transTypeStr, "NONE") != 0 && strcasecmp(transTypeStr, "DIRECT") != 0) {
            LOG_CFG_WARN("transmission: unknown type '%s' — defaulting to none", transTypeStr);
        }
        cfg.transmission.type = RcEngineSound::TRANS_NONE;
    }

    cfg.transmission.numberOfGears = tr["number_of_gears"] | tr["NUMBER_OF_GEARS"] | 3;
    cfg.transmission.torqueConverterSlip = tr["torque_converter_slip"] | tr["TORQUE_CONVERTER_SLIP"] | 100;

    JsonArrayConst rampTimes = tr["gear_ramp_times"] | tr["GEAR_RAMP_TIMES"];
    if (rampTimes) {
        int idx = 0;
        for (int val : rampTimes) {
            if (idx < 6) cfg.transmission.gearRampTimes[idx++] = val;
        }
    }
}

void ConfigParser::parseFeatures(JsonDocument& doc, RcEngineSound::Config& cfg) {
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

void ConfigParser::parseLoopPoints(JsonDocument& doc, RcEngineSound::Config& cfg) {
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

void ConfigParser::parseMixWeights(JsonDocument& doc, RcEngineSound::Config& cfg) {
    JsonVariantConst mw = doc["mix_weights"] | doc["MIX_WEIGHTS"];
    cfg.sound.engineMixWeight = mw["engine"] | mw["ENGINE"] | 100;
    cfg.sound.effectMixWeight = mw["effects"] | mw["EFFECTS"] | 100;
}

void ConfigParser::validateVehicleConfig(const RcEngineSound::Config& cfg) {
    if (cfg.transmission.numberOfGears < 1 || cfg.transmission.numberOfGears > 6) {
        LOG_CFG_WARN("transmission: number_of_gears %d out of range (1-6)",
                     cfg.transmission.numberOfGears);
    }
}
