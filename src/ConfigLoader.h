#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "RcEngineSound.h"

class ConfigLoader {
public:
    static bool begin() {
        if (!LittleFS.begin(true)) {
            Serial.println("LittleFS Mount Failed");
            return false;
        }
        Serial.println("LittleFS Mounted Successfully");
        return true;
    }

    static bool loadConfig(const char* path, RcEngineSound::Config& cfg) {
        File file = LittleFS.open(path, "r");
        if (!file) {
            Serial.printf("Failed to open %s\n", path);
            return false;
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, file);
        if (error) {
            Serial.printf("JSON Parse Failed: %s\n", error.c_str());
            file.close();
            return false;
        }

        // 1. Engine settings
        JsonObject engine = doc["engine"];
        if (!engine.isNull()) {
            if (engine["acc"]) cfg.acc = engine["acc"];
            if (engine["dec"]) cfg.dec = engine["dec"];
            if (engine["inertia"]) cfg.inertia = engine["inertia"];
            if (engine["maxRpm"]) cfg.maxRpm = engine["maxRpm"];
            if (engine["maxRpmPercentage"]) cfg.maxRpmPercentage = engine["maxRpmPercentage"];
            if (engine["clutchEngagingPoint"]) cfg.clutchEngagingPoint = engine["clutchEngagingPoint"];
            if (engine["revSwitchPoint"]) cfg.revSwitchPoint = engine["revSwitchPoint"];
            if (engine["idleEndPoint"]) cfg.idleEndPoint = engine["idleEndPoint"];
        }
        
        // 2. Transmission settings
        JsonObject trans = doc["transmission"];
        if (!trans.isNull()) {
            if (trans["automatic"]) cfg.automatic = trans["automatic"];
        }

        // 3. Sound settings
        JsonObject sounds = doc["sounds"];
        if (!sounds.isNull()) {
            if (sounds["masterVolume"]) cfg.masterVolume = sounds["masterVolume"];
            if (sounds["startVolume"]) cfg.startVolume = sounds["startVolume"];
            if (sounds["idleVolume"]) cfg.idleVolume = sounds["idleVolume"];
            if (sounds["revVolume"]) cfg.revVolume = sounds["revVolume"];
            if (sounds["turboVolume"]) cfg.turboVolume = sounds["turboVolume"];
            if (sounds["knockVolume"]) cfg.knockVolume = sounds["knockVolume"];
            if (sounds["wastegateVolume"]) cfg.wastegateVolume = sounds["wastegateVolume"];
            if (sounds["hornVolume"]) cfg.hornVolume = sounds["hornVolume"];
            if (sounds["fanVolume"]) cfg.fanVolume = sounds["fanVolume"];
            if (sounds["jakeBrakeVolume"]) cfg.jakeBrakeVolume = sounds["jakeBrakeVolume"];
            if (sounds["shiftingVolume"]) cfg.shiftingVolume = sounds["shiftingVolume"];
            if (sounds["brakeVolume"]) cfg.brakeVolume = sounds["brakeVolume"];
            if (sounds["reversingVolume"]) cfg.reversingVolume = sounds["reversingVolume"];
            if (sounds["sirenVolume"]) cfg.sirenVolume = sounds["sirenVolume"];
            if (sounds["parkingBrakeVolume"]) cfg.parkingBrakeVolume = sounds["parkingBrakeVolume"];
        }

        // 4. Light settings
        JsonObject lights = doc["lights"];
        if (!lights.isNull()) {
            if (lights["xenon"]) cfg.lights.xenon = lights["xenon"];
            if (lights["doubleFlashBlue"]) cfg.lights.doubleFlashBlue = lights["doubleFlashBlue"];
        }

        file.close();
        Serial.printf("Loaded config from %s\n", path);
        return true;
    }

    static int8_t* loadSoundFile(const char* path, uint32_t& count) {
        File file = LittleFS.open(path, "r");
        if (!file) {
            Serial.printf("Sound file not found: %s\n", path);
            count = 0;
            return nullptr;
        }

        size_t size = file.size();
        // Allocate in PSRAM
        int8_t* buffer = (int8_t*)ps_malloc(size);
        if (!buffer) {
            Serial.printf("Failed to allocate %d bytes in PSRAM for %s\n", size, path);
            file.close();
            count = 0;
            return nullptr;
        }

        file.readBytes((char*)buffer, size);
        file.close();
        
        count = size;
        Serial.printf("Loaded %s (%d bytes) into PSRAM\n", path, size);
        return buffer;
    }

    static bool loadAllSounds(SoundData& data) {
        data.isDynamic = true;
        data.samples = loadSoundFile("/sounds/idle.raw", data.sampleCount);
        data.startSamples = loadSoundFile("/sounds/start.raw", data.startSampleCount);
        data.revSamples = loadSoundFile("/sounds/rev.raw", data.revSampleCount);
        data.turboSamples = loadSoundFile("/sounds/turbo.raw", data.turboSampleCount);
        data.knockSamples = loadSoundFile("/sounds/knock.raw", data.knockSampleCount);
        data.wastegateSamples = loadSoundFile("/sounds/wastegate.raw", data.wastegateSampleCount);
        data.hornSamples = loadSoundFile("/sounds/horn.raw", data.hornSampleCount);
        
        return (data.samples != nullptr); // At least idle must exist
    }
};
