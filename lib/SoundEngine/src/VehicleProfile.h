#pragma once

#include <Arduino.h>
#include <RcEngineSound.h>
#include <SoundTypes.h>

struct VehicleProfile {
    RcEngineSound::Config config;
    SoundData sounds;
    bool loaded = false;

    void reset() {
        config = RcEngineSound::Config();
        sounds = SoundData();
        loaded = false;
    }

    RcEngineSound::Config& getConfig() { return config; }
    SoundData& getSounds() { return sounds; }
};
