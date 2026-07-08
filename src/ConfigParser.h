#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "PinMapper.h"

class ConfigParser {
public:
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

        if (doc.containsKey("SOUND")) {
            config.sound.volume = doc["SOUND"]["VOLUME"] | 80;
        }

        if (doc.containsKey("DRIVE_MOTOR")) {
            parseDriveMotor(doc["DRIVE_MOTOR"], config.driveMotor);
        }

        if (doc.containsKey("STEERING_SERVO")) {
            parseSteeringServo(doc["STEERING_SERVO"], config.steeringServo);
        }

        if (doc.containsKey("LIGHTS")) {
            parseLights(doc["LIGHTS"], config.lights);
        }

        Serial.printf("[ConfigParser] Loaded hardware config: %s\n", path);
        return true;
    }

    static bool loadVehicleConfig(const char* path, VehicleConfig& config) {
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

        config = VehicleConfig();

        if (doc.containsKey("VEHICLE")) {
            strlcpy(config.name, doc["VEHICLE"]["NAME"] | "Unknown", sizeof(config.name));
            strlcpy(config.type, doc["VEHICLE"]["TYPE"] | "UNKNOWN", sizeof(config.type));
        }

        if (doc.containsKey("ENGINE")) {
            parseEngine(doc["ENGINE"], config.engine);
        }

        if (doc.containsKey("TRANSMISSION")) {
            parseTransmission(doc["TRANSMISSION"], config.transmission);
        }

        if (doc.containsKey("SOUND_VOLUME")) {
            parseSoundVolume(doc["SOUND_VOLUME"], config.soundVolume);
        }

        Serial.printf("[ConfigParser] Loaded vehicle config: %s (%s)\n", config.name, config.type);
        return true;
    }

private:
    static void parseDriveMotor(JsonObject motor, HardwareConfig::DriveMotor& config) {
        const char* hw = motor["HARDWARE"] | "";
        if (strcmp(hw, "HBRIDGE_A") == 0 || strcmp(hw, "HBRIDGE_B") == 0) {
            config.type = HardwareConfig::DriveMotor::HBRIDGE;
        } else if (hw[0] == 'S') {
            config.type = HardwareConfig::DriveMotor::ESC;
        }
        config.hardwareId = PinMapper::resolve(hw);
        config.frequency = motor["FREQUENCY"] | 20000;

        const char* dir = motor["DIRECTION"] | "FORWARD";
        if (strcmp(dir, "REVERSE") == 0) config.direction = HardwareConfig::DriveMotor::REVERSE;
        else if (strcmp(dir, "UNI_FORWARD") == 0) config.direction = HardwareConfig::DriveMotor::UNI_FORWARD;
        else if (strcmp(dir, "UNI_REVERSE") == 0) config.direction = HardwareConfig::DriveMotor::UNI_REVERSE;
        else config.direction = HardwareConfig::DriveMotor::FORWARD;

        config.duty.min = motor["DUTY"]["MIN"] | 20;
        config.duty.max = motor["DUTY"]["MAX"] | 90;
    }

    static void parseSteeringServo(JsonObject servo, HardwareConfig::SteeringServo& config) {
        const char* hw = servo["HARDWARE"] | "";
        config.hardwareId = PinMapper::resolve(hw);
        config.frequency = servo["FREQUENCY"] | 50;
        config.endpoints.left = servo["ENDPOINTS"]["LEFT"] | 1350;
        config.endpoints.right = servo["ENDPOINTS"]["RIGHT"] | 1650;
        config.endpoints.center = servo["ENDPOINTS"]["CENTER"] | 1500;
    }

    static void parseLights(JsonObject lights, HardwareConfig::Lights& config) {
        if (lights.containsKey("HEAD_LIGHT")) {
            config.headLight.pin = PinMapper::resolve(lights["HEAD_LIGHT"]["HARDWARE"] | "");
            config.headLight.brightness = lights["HEAD_LIGHT"]["BRIGHTNESS_MAX"] | 60;
            config.headLight.configured = config.headLight.pin != 0xFF;
        }

        if (lights.containsKey("TAIL_LIGHT")) {
            config.tailLight.pin = PinMapper::resolve(lights["TAIL_LIGHT"]["HARDWARE"] | "");
            config.tailLight.brightness = lights["TAIL_LIGHT"]["BRIGHTNESS_MAX"] | 60;
            config.tailLight.configured = config.tailLight.pin != 0xFF;
        }

        if (lights.containsKey("BRAKE_LIGHT")) {
            config.brakeLight.pin = PinMapper::resolve(lights["BRAKE_LIGHT"]["HARDWARE"] | "");
            config.brakeLight.brightness = 100;
            config.brakeLight.configured = config.brakeLight.pin != 0xFF;
        }

        if (lights.containsKey("TURN_LIGHT")) {
            config.turnLight.leftPin = PinMapper::resolve(lights["TURN_LIGHT"]["LEFT"]["HARDWARE"] | "");
            config.turnLight.rightPin = PinMapper::resolve(lights["TURN_LIGHT"]["RIGHT"]["HARDWARE"] | "");
            config.turnLight.brightness = lights["TURN_LIGHT"]["BRIGHTNESS_MAX"] | 60;
            config.turnLight.intervalOn = lights["TURN_LIGHT"]["INTERVAL_ON"] | 500;
            config.turnLight.intervalOff = lights["TURN_LIGHT"]["INTERVAL_OFF"] | 500;
            config.turnLight.configured = config.turnLight.leftPin != 0xFF || config.turnLight.rightPin != 0xFF;
        }

        if (lights.containsKey("REVERSING_LIGHT")) {
            config.reversingLight.pin = PinMapper::resolve(lights["REVERSING_LIGHT"]["HARDWARE"] | "");
            config.reversingLight.brightness = 100;
            config.reversingLight.configured = config.reversingLight.pin != 0xFF;
        }
    }

    static void parseEngine(JsonObject engine, VehicleConfig::Engine& config) {
        config.acc = engine["ACCELERATION"] | 2;
        config.dec = engine["DECELERATION"] | 1;
        config.idleRpm = engine["IDLE_RPM"] | 10;
        config.clutchRpm = engine["CLUTCH_RPM"] | 100;
        config.revSwitchPoint = engine["REV_SWITCH_POINT"] | 50;
        config.idleEndPoint = engine["IDLE_END_POINT"] | 40;
        config.knockInterval = engine["DIESEL_KNOCK_INTERVAL"] | 8;
        config.knockStartPoint = engine["DIESEL_KNOCK_START_POINT"] | 30;
        config.jakeBrakeMinRpm = engine["JAKEBRAKE_MIN_RPM"] | 60;
        config.fanStartPoint = engine["FAN_START_POINT"] | 0;
    }

    static void parseTransmission(JsonObject trans, VehicleConfig::Transmission& config) {
        const char* type = trans["TYPE"] | "NONE";
        if (strcmp(type, "AUTOMATIC") == 0) config.type = VehicleConfig::Transmission::AUTOMATIC;
        else if (strcmp(type, "MANUAL") == 0) config.type = VehicleConfig::Transmission::MANUAL;
        else config.type = VehicleConfig::Transmission::NONE;
        config.numberOfGears = trans["NUMBER_OF_GEARS"] | 3;
    }

    static void parseSoundVolume(JsonObject vol, VehicleConfig::SoundVolume& config) {
        config.start = vol["START"] | 100;
        config.idle = vol["IDLE"] | 100;
        config.engineIdle = vol["ENGINE_IDLE"] | 50;
        config.fullThrottle = vol["FULL_THROTTLE"] | 150;
        config.rev = vol["REV"] | 100;
        config.engineRev = vol["ENGINE_REV"] | 50;
        config.turbo = vol["TURBO"] | 0;
        config.knock = vol["KNOCK"] | 0;
        config.wastegate = vol["WASTEGATE"] | 0;
        config.horn = vol["HORN"] | 100;
        config.siren = vol["SIREN"] | 0;
        config.brake = vol["BRAKE"] | 0;
        config.parkingBrake = vol["PARKING_BRAKE"] | 0;
        config.shifting = vol["SHIFTING"] | 0;
        config.reversing = vol["REVERSING"] | 0;
        config.indicator = vol["INDICATOR"] | 100;
        config.coupling = vol["COUPLING"] | 100;
        config.jakeBrake = vol["JAKEBRAKE"] | 0;
        config.fan = vol["FAN"] | 0;
    }
};
