#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "PinMapper.h"
#include "UiLogger.h"
#include <RcEngineSound.h>
#include <SoundTypes.h>

#define LOG_CFG_WARN(fmt, ...) do { \
    Serial.printf("[ConfigParser] WARN: " fmt "\n", ##__VA_ARGS__); \
    UiLogger::logf("WARN: " fmt, ##__VA_ARGS__); \
} while(0)

#define LOG_CFG_ERR(fmt, ...) do { \
    Serial.printf("[ConfigParser] ERR: " fmt "\n", ##__VA_ARGS__); \
    UiLogger::logf("ERR: " fmt, ##__VA_ARGS__); \
} while(0)

#pragma pack(push, 1)
struct PcmHeader {
    char     magic[2];    // "RP" (0x52, 0x50)
    uint16_t sampleRate;  // Little-Endian (default: 22050)
    uint32_t sampleCount; // Little-Endian byte count of PCM data
};
#pragma pack(pop)

class ConfigParser {
public:
    static const char* soundTypeNames[];
    static const char* genericNames[];

    static bool begin();
    static bool loadHardwareConfig(const char* path, HardwareConfig& config);
    static bool loadVehicleConfig(const char* path, RcEngineSound::Config& config);
    static bool loadSounds(const RcEngineSound::Config& cfg, SoundData& soundData);
    static bool loadSounds(const char* vehicleName, SoundData& soundData);
    static void printFileTree(const char* dirPath = "/", int depth = 0);
    static void printFilesystemInfo();

    static SoundSlot loadSoundSlot(const char* path);

    static bool keyAllowed(const char* key, const char* const* allowed, size_t n);
    static void checkKeys(JsonObjectConst obj, const char* section,
                          const char* const* allowed, size_t n);
    static void warnUnresolvedHardware(const char* where, const char* hw);
    static void checkUnknownHardwareKeys(JsonObjectConst doc);
    static void validateHardwareConfig(const HardwareConfig& cfg);

    static void parseDriveMotor(JsonVariantConst motor, HardwareConfig::DriveMotor& config);
    static void parseSteeringServo(JsonVariantConst servo, HardwareConfig::SteeringServo& config);
    static uint8_t resolveLightAlias(const char* hw, const HardwareConfig& config);
    static void parseLight(JsonVariantConst light, HardwareConfig::Lights::Light& config, const char* name, uint8_t defaultBrightness = 60);
    static void parseLights(JsonObjectConst lights, HardwareConfig::Lights& config);
    static void parseAuxMotor(JsonVariantConst motor, HardwareConfig::AuxMotor& config);
    static void parseAuxLight(JsonVariantConst light, HardwareConfig::AuxLight& config);

    static void checkUnknownVehicleKeys(JsonObjectConst doc);
    static void parseEngine(JsonDocument& doc, RcEngineSound::Config& cfg);
    static void parseSoundVolumes(JsonDocument& doc, RcEngineSound::Config& cfg);
    static void parseTransmission(JsonDocument& doc, RcEngineSound::Config& cfg);
    static void parseFeatures(JsonDocument& doc, RcEngineSound::Config& cfg);
    static void parseLoopPoints(JsonDocument& doc, RcEngineSound::Config& cfg);
    static void parseMixWeights(JsonDocument& doc, RcEngineSound::Config& cfg);
    static void validateVehicleConfig(const RcEngineSound::Config& cfg);
};
