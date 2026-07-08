#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <RcEngineSound.h>

class SoundLoader {
public:
    static bool begin() {
        if (!LittleFS.begin(true)) {
            Serial.println("[SoundLoader] LittleFS mount failed");
            return false;
        }
        Serial.println("[SoundLoader] LittleFS mounted");
        return true;
    }

    static SoundData* loadSound(const char* path) {
        File file = LittleFS.open(path, "r");
        if (!file) {
            Serial.printf("[SoundLoader] Cannot open: %s\n", path);
            return nullptr;
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, file);
        file.close();

        if (error) {
            Serial.printf("[SoundLoader] JSON error in %s: %s\n", path, error.c_str());
            return nullptr;
        }

        uint32_t sampleCount = doc["sampleCount"] | 0;
        uint16_t sampleRate = doc["sampleRate"] | 22050;
        JsonArray samples = doc["samples"].as<JsonArray>();

        if (sampleCount == 0 || !samples) {
            Serial.printf("[SoundLoader] Invalid data in %s\n", path);
            return nullptr;
        }

        int8_t* buffer = (int8_t*)ps_malloc(sampleCount);
        if (!buffer) {
            Serial.printf("[SoundLoader] PSRAM alloc failed (%u bytes)\n", sampleCount);
            return nullptr;
        }

        uint32_t i = 0;
        for (int val : samples) {
            buffer[i++] = (int8_t)val;
        }

        SoundData* sd = new SoundData();
        sd->samples = buffer;
        sd->sampleCount = sampleCount;
        sd->sampleRate = sampleRate;
        sd->isDynamic = true;

        Serial.printf("[SoundLoader] Loaded %s: %u samples @ %u Hz (%u bytes PSRAM)\n",
                      path, sampleCount, sampleRate, sampleCount);
        return sd;
    }

    static void unloadSound(SoundData* sd) {
        if (!sd) return;
        if (sd->isDynamic) {
            free(sd->samples);
            sd->samples = nullptr;
        }
        delete sd;
    }

    static void unloadAll(SoundData* sounds[], int count) {
        for (int i = 0; i < count; i++) {
            unloadSound(sounds[i]);
            sounds[i] = nullptr;
        }
    }
};
