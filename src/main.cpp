#include <Arduino.h>
#include "boards.h"  // board selected at build time via platformio.ini env define
#include "Config.h"
#include "ConfigParser.h"
#include "HardwareInit.h"
#include "RADIOKIT.h"
#include "VehicleController.h"
#include <RcEngineSound.h>
#include <VehicleProfile.h>
#include <AudioOutput.h>

RcEngineSound engine;
VehicleProfile profile;
HardwareConfig hwConfig;

// ── Config hot-reload state ──
static uint32_t lastCfgCheck = 0;
static uint32_t lastHwWrite = 0;
static uint32_t lastVcWrite = 0;
// Debounce: a config upload rewrites the file over several frames (RadioKit FS
// upload: truncate -> chunks -> end). Triggering reload on the first mtime
// change reads a partial/empty file. Wait for the mtime to be stable across
// two consecutive polls before reloading.
static bool     pendingReload = false;
static uint32_t pendingHwT = 0;
static uint32_t pendingVcT = 0;

static uint32_t fileWriteTime(const char* path) {
    File f = LittleFS.open(path, "r");
    if (!f) return 0;
    uint32_t t = (uint32_t)f.getLastWrite();
    f.close();
    return t;
}

static bool reloadConfigs() {
    Serial.println("\n── Reloading Configs ──");

    HardwareConfig newHw;
    RcEngineSound::Config newVc;

    if (!ConfigParser::loadHardwareConfig("/hardware-config.json", newHw)) {
        Serial.println("[Reload] hardware-config.json invalid — keeping current config");
        return false;
    }
    if (!ConfigParser::loadVehicleConfig("/vehicle-config.json", newVc)) {
        Serial.println("[Reload] vehicle-config.json invalid — keeping current config");
        return false;
    }

    bool nameChanged = strcmp(newVc.name, profile.config.name) != 0;

    hwConfig = newHw;
    profile.config = newVc;
    profile.config.sound.master = hwConfig.sound.volume;

    HardwareInit::hotReload(hwConfig);
    engine.setConfig(profile.config);

    if (nameChanged) {
        ConfigParser::loadSounds(profile.config, profile.sounds);
        engine.begin(profile.sounds);
    }

    Serial.println("[Reload] Configs reloaded OK");
    return true;
}

void printConfig(const HardwareConfig& hw, const RcEngineSound::Config& vc) {
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
    Serial.printf("  Name: %s (Type: %d)\n", vc.name, static_cast<int>(vc.type));
    Serial.printf("  Engine: acc=%d dec=%d idle=%d clutch=%d\n",
                  vc.engine.acc, vc.engine.dec, vc.engine.idleEndPoint, vc.engine.revSwitchPoint);
    Serial.printf("  Transmission: type=%d gears=%d\n",
                  vc.transmission.type, vc.transmission.numberOfGears);
    Serial.printf("  Sound: start=%d idle=%d rev=%d turbo=%d knock=%d horn=%d\n",
                  vc.sound.start, vc.sound.idle, vc.sound.rev,
                  vc.sound.turbo, vc.sound.knock, vc.sound.horn);
}

void setup() {
    // HWCDC RX buffer defaults to 256 bytes — too small for a single
    // RadioKit FS-upload frame (~KB scale), which caused dropped/corrupted
    // chunks. Enlarge before begin() so serial protocol frames arrive intact.
    Serial.setRxBufferSize(8192);
    Serial.begin(2000000);
    delay(1000);

    Serial.println("\n=== RC Brain - Unified Vehicle Controller ===\n");

    if (!ConfigParser::begin()) {
        Serial.println("FATAL: LittleFS mount failed");
        while (1) delay(100);
    }

    ConfigParser::printFilesystemInfo();

    Serial.println("\n── Loading Configs ──");
    bool hwOk = ConfigParser::loadHardwareConfig("/hardware-config.json", hwConfig);
    bool vcOk = ConfigParser::loadVehicleConfig("/vehicle-config.json", profile.config);

    if (!hwOk || !vcOk) {
        Serial.println("\nFATAL ERROR: Failed to load /hardware-config.json or /vehicle-config.json!");
        Serial.println("Execution halted. Please upload valid configuration files to LittleFS.");
        while (1) delay(100);
    }

    printConfig(hwConfig, profile.config);

    Serial.println("\n── Loading Sounds ──");
    ConfigParser::loadSounds(profile.config, profile.sounds);

    Serial.println("\n── Initializing Hardware ──");
    HardwareInit::init(hwConfig);

    Serial.println("\n── Initializing Vehicle Controller ──");
    VehicleController::init(&hwConfig, &engine, &profile);

    Serial.println("\n── Starting Engine Sound ──");
    engine.setConfig(profile.config);
    engine.begin(profile.sounds);
    AudioOutput::begin(&engine);
    AudioOutput::start();

    Serial.println("\n── Starting RadioKit (BLE) ──");
    initRadioKit();

    Serial.println("\n── System Ready ──");
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
    Serial.println("\n=== Init Complete ===\n");

    // Seed the hot-reload watcher so boot does not trigger a spurious reload
    // (file write times are nonzero while lastHwWrite/lastVcWrite start at 0).
    lastHwWrite = fileWriteTime("/hardware-config.json");
    lastVcWrite = fileWriteTime("/vehicle-config.json");
}

void loop() {
    RadioKit.update();
    VehicleController::update();

    // Watch for config changes saved via the RadioKit filesystem manager
    uint32_t now = millis();
    if (now - lastCfgCheck >= 2000) {
        lastCfgCheck = now;
        uint32_t hwT = fileWriteTime("/hardware-config.json");
        uint32_t vcT = fileWriteTime("/vehicle-config.json");
        bool changed = (hwT != lastHwWrite) || (vcT != lastVcWrite);
        if (changed && !pendingReload) {
            // First sighting of a change — remember it and wait one poll.
            pendingReload = true;
            pendingHwT = hwT;
            pendingVcT = vcT;
        } else if (pendingReload) {
            if (hwT == pendingHwT && vcT == pendingVcT) {
                // mtime stable across two polls — the upload has finished.
                pendingReload = false;
                lastHwWrite = hwT;
                lastVcWrite = vcT;
                reloadConfigs();
            } else {
                // Still being written — keep waiting.
                pendingHwT = hwT;
                pendingVcT = vcT;
            }
        }
    }
}
