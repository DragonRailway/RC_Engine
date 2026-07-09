#include <Arduino.h>
#include "TRACKLINK_V3.h"
#include "ConfigLoader.h"
#include "Config.h"
#include "ConfigParser.h"
#include "HardwareInit.h"
#include "SoundLoader.h"
#include "VehicleProfile.h"
#include "AudioOutput.h"
#include <RcEngineSound.h>

RcEngineSound engine;
VehicleProfile profile;
SoundData soundData;
HardwareConfig hwConfig;
VehicleConfig vehicleConfig;

void printConfig(const HardwareConfig& hw, const VehicleConfig& vc) {
    Serial.println("\n── Hardware Config ──");
    Serial.printf("  Sound Volume: %d%%\n", hw.sound.volume);
    Serial.printf("  Drive Motor: type=%d hwId=%d freq=%dHz dir=%d\n",
                  hw.driveMotor.type, hw.driveMotor.hardwareId,
                  hw.driveMotor.frequency, hw.driveMotor.direction);
    Serial.printf("  Steering Servo: hwId=%d freq=%dHz L=%d R=%d C=%d\n",
                  hw.steeringServo.hardwareId, hw.steeringServo.frequency,
                  hw.steeringServo.endpoints.left, hw.steeringServo.endpoints.right,
                  hw.steeringServo.endpoints.center);
    Serial.printf("  Lights: head=%d tail=%d brake=%d turnL=%d turnR=%d\n",
                  hw.lights.headLight.pin, hw.lights.tailLight.pin,
                  hw.lights.brakeLight.pin, hw.lights.turnLight.leftPin,
                  hw.lights.turnLight.rightPin);

    Serial.println("\n── Vehicle Config ──");
    Serial.printf("  Name: %s Type: %s\n", vc.name, vc.type);
    Serial.printf("  Engine: acc=%d dec=%d idle=%d clutch=%d\n",
                  vc.engine.acc, vc.engine.dec, vc.engine.idleRpm, vc.engine.clutchRpm);
    Serial.printf("  Transmission: type=%d gears=%d\n", vc.transmission.type, vc.transmission.numberOfGears);
    Serial.printf("  Sound: start=%d idle=%d rev=%d turbo=%d knock=%d horn=%d\n",
                  vc.soundVolume.start, vc.soundVolume.idle, vc.soundVolume.rev,
                  vc.soundVolume.turbo, vc.soundVolume.knock, vc.soundVolume.horn);
}

void setup() {
    Serial.begin(2000000);
    delay(1000);

    Serial.println("\n=== RC Brain - Locomotive Controller ===\n");

    if (!SoundLoader::begin()) {
        Serial.println("FATAL: LittleFS mount failed");
        while (1) delay(100);
    }

    ConfigLoader::printFilesystemInfo();

    Serial.println("\n── Loading Configs ──");
    bool hwOk = ConfigParser::loadHardwareConfig("/hardware-config.json", hwConfig);
    bool vcOk = ConfigParser::loadVehicleConfig("/vehicle-config.json", vehicleConfig);

    if (hwOk && vcOk) {
        printConfig(hwConfig, vehicleConfig);

        Serial.println("\n── Initializing Hardware ──");
        HardwareInit::init(hwConfig);

        Serial.println("\n── Loading Vehicle Profile ──");
        if (profile.load("/vehicle-config.json")) {
            profile.populateSoundData(soundData);
            RcEngineSound::Config config;
            profile.populateConfig(config);
            engine.setConfig(config);

            engine.begin(soundData);
            AudioOutput::begin(&engine);
            AudioOutput::start();

            Serial.println("\n── System Ready ──");
            Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
            Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
        } else {
            Serial.println("Failed to load vehicle profile");
        }
    } else {
        Serial.println("Failed to load configs");
    }

    Serial.println("\n=== Init Complete ===\n");
}

void loop() {
    delay(1000);
}
