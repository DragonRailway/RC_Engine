#include <Arduino.h>
#include "boards.h"  // board selected at build time via platformio.ini env define
#include "Config.h"
#include "RADIOKIT.h"
#include "UiLogger.h"
#include "ConfigParser.h"
#include "HardwareInit.h"
#include "VehicleController.h"
#include <RcEngineSound.h>
#include <VehicleProfile.h>
#include <AudioOutput.h>

// ── Board-specific hardware config ──
// Each board has its own hardware config file on LittleFS, selected here at
// compile time by the same -D define that selects the board pins. The shared
// /hardware-config.json is no longer used; each board tunes its own file
// (e.g. voltage calibration, servo endpoints, battery) independently.
#ifdef MIKRO_V2
#define HW_CONFIG_PATH "/hardware-MIKRO_V2.json"
#elif defined(TRACKLINK_V3)
#define HW_CONFIG_PATH "/hardware-TRACKLINK_V3.json"
#else
#error "Unknown board: define MIKRO_V2 or TRACKLINK_V3 in platformio.ini"
#endif

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

// Aux slider control profile from aux_motor.type. mixer: 5 detents, no
// self-centering (the user sets a speed/direction that keeps running); tipper:
// no detents, self-centering (momentary — follows the finger). trailer_dcc
// leaves the default (no aux channel), matching the deferred state.
// Must run after config load and before the app connects (RadioKit sends the
// widget config on connect).
static void applyAuxSliderProfile() {
    if (hwConfig.auxMotor.purpose == HardwareConfig::AuxMotor::MIXER) {
        aux_slider.rk.centering = RK_SPRING_NONE;
        aux_slider.rk.detents = 5;
    } else if (hwConfig.auxMotor.purpose == HardwareConfig::AuxMotor::TIPPER) {
        aux_slider.rk.centering = RK_SPRING_CENTER;
        aux_slider.rk.detents = 0;
    }
    // trailer_dcc / no aux_motor: leave the generated defaults (mixer-shaped);
    // the channel itself is unconfigured with a parser warning.
}

static uint32_t fileWriteTime(const char* path) {
    File f = LittleFS.open(path, "r");
    if (!f) return 0;
    uint32_t t = (uint32_t)f.getLastWrite();
    f.close();
    return t;
}

#if defined(RK_ENABLE_BLE)
#include <NimBLEDevice.h>
#endif

// ── Device Metadata Cascade ──
// Priority:
// 1) Hardware config (board-specific name / description)
// 2) Vehicle config (bundle vehicle.name / vehicle.description)
// 3) Fallback defaults ("RC_UI" / "")
static void applyDeviceMetadata(const HardwareConfig& hw, const RcEngineSound::Config& vc) {
    if (hw.name[0] != '\0') {
        RadioKit.config.name = hw.name;
    } else if (vc.name[0] != '\0' && strcmp(vc.name, "Unknown") != 0) {
        RadioKit.config.name = vc.name;
    } else {
        RadioKit.config.name = "RC_UI";
    }

    if (hw.description[0] != '\0') {
        RadioKit.config.description = hw.description;
    } else if (vc.description[0] != '\0') {
        RadioKit.config.description = vc.description;
    } else {
        RadioKit.config.description = "";
    }

    if (vc.type == RcEngineSound::VEHICLE_LOCOMOTIVE) {
        RadioKit.config.type = "Locomotive";
        RadioKit.setActivePage(1);   // page 1 "Loco"
    } else {
        RadioKit.config.type = "Truck";
        RadioKit.setActivePage(0);   // page 0 "Truck"
    }

#if defined(RK_ENABLE_BLE)
    NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
    if (pAdv) {
        pAdv->setName(RadioKit.config.name);
    }
#endif

    Serial.printf("[Device] Name: '%s', Description: '%s', Type: '%s'\n",
                  RadioKit.config.name, RadioKit.config.description, RadioKit.config.type);
}

static bool reloadConfigs() {
    Serial.println("\n── Reloading Configs ──");
    UiLogger::clear();

    HardwareConfig newHw;
    RcEngineSound::Config newVc;

    if (!ConfigParser::loadHardwareConfig(HW_CONFIG_PATH, newHw)) {
        Serial.printf("[Reload] %s invalid — keeping current config\n", HW_CONFIG_PATH);
        UiLogger::logf("ERR: %s invalid", HW_CONFIG_PATH);
        return false;
    }
    if (!ConfigParser::loadVehicleConfig("/vehicle-config.json", newVc)) {
        Serial.println("[Reload] vehicle-config.json invalid — keeping current config");
        UiLogger::logf("ERR: vehicle-config.json invalid");
        return false;
    }

    bool nameChanged = strcmp(newVc.name, profile.config.name) != 0;

    hwConfig = newHw;
    profile.config = newVc;
    profile.config.sound.master = hwConfig.sound.volume;

    HardwareInit::hotReload(hwConfig);
    VehicleController::applyConfiguredLightMask(hwConfig.lights, hwConfig.auxLight.configured);
    engine.setConfig(profile.config);
    applyAuxSliderProfile();
    applyDeviceMetadata(hwConfig, profile.config);

    if (nameChanged) {
        ConfigParser::loadSounds(profile.config, profile.sounds);
        engine.begin(profile.sounds);
    }

    if (!UiLogger::hasErrors()) {
        UiLogger::clear();
    }

    Serial.println("[Reload] Configs reloaded OK");
    return true;
}

void printConfig(const HardwareConfig& hw, const RcEngineSound::Config& vc) {
    Serial.println("\n── Hardware Config ──");
    if (hw.name[0] != '\0') Serial.printf("  Name: %s\n", hw.name);
    if (hw.description[0] != '\0') Serial.printf("  Description: %s\n", hw.description);
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
    if (vc.description[0] != '\0') Serial.printf("  Description: %s\n", vc.description);
    Serial.printf("  Engine: acc=%d dec=%d idle=%d clutch=%d\n",
                  vc.engine.acc, vc.engine.dec, vc.engine.idleEndPoint, vc.engine.revSwitchPoint);
    Serial.printf("  Transmission: type=%d gears=%d\n",
                  vc.transmission.type, vc.transmission.numberOfGears);
    Serial.printf("  Sound: start=%d idle=%d rev=%d turbo=%d knock=%d horn=%d\n",
                  vc.sound.start, vc.sound.idle, vc.sound.rev,
                  vc.sound.turbo, vc.sound.knock, vc.sound.horn);
}

void setup() {
    HardwareInit::latchPower();
    UiLogger::init();

    // HWCDC RX buffer defaults to 256 bytes — too small for a single
    // RadioKit FS-upload frame (~KB scale), which caused dropped/corrupted
    // chunks. Enlarge before begin() so serial protocol frames arrive intact.
    Serial.setRxBufferSize(8192);
    Serial.begin(2000000);
#if ARDUINO_USB_CDC_ON_BOOT || ARDUINO_USB_MODE
    Serial.setTxTimeoutMs(0);
#endif
    delay(1000);

    Serial.println("\n=== RC Brain - Unified Vehicle Controller ===\n");

    if (!ConfigParser::begin()) {
        Serial.println("FATAL: LittleFS mount failed");
        UiLogger::log("FATAL: LittleFS mount failed");
        while (1) delay(100);
    }

    ConfigParser::printFilesystemInfo();

    Serial.println("\n── Loading Configs ──");
    bool hwOk = ConfigParser::loadHardwareConfig(HW_CONFIG_PATH, hwConfig);
    bool vcOk = ConfigParser::loadVehicleConfig("/vehicle-config.json", profile.config);

    if (!hwOk || !vcOk) {
        Serial.printf("\nFATAL ERROR: Failed to load %s or /vehicle-config.json!\n", HW_CONFIG_PATH);
        Serial.println("Execution halted. Please upload valid configuration files to LittleFS.");
        UiLogger::logf("FATAL: Failed to load config!");
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
    applyDeviceMetadata(hwConfig, profile.config);

    RadioKit.begin();
    RadioKit.startSerial(Serial);
    RadioKit.startBLE();
    RadioKit.enableFS();

    UiLogger::onRadioKitStarted();
    applyAuxSliderProfile();
    VehicleController::applyConfiguredLightMask(hwConfig.lights, hwConfig.auxLight.configured);

    Serial.println("\n── System Ready ──");
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
    Serial.println("\n=== Init Complete ===\n");

    // Seed the hot-reload watcher so boot does not trigger a spurious reload
    // (file write times are nonzero while lastHwWrite/lastVcWrite start at 0).
    lastHwWrite = fileWriteTime(HW_CONFIG_PATH);
    lastVcWrite = fileWriteTime("/vehicle-config.json");
}

void loop() {
    RadioKit.update();
    VehicleController::update();
    // Pump the EasyKit animation engines (servo easing, LED fades, blink
    // patterns) so they advance non-blocking every iteration.
    HardwareInit::update(hwConfig.power.buttonHoldS, hwConfig.power.indicatorPin);

    // Watch for config changes saved via the RadioKit filesystem manager
    uint32_t now = millis();
    if (now - lastCfgCheck >= 2000) {
        lastCfgCheck = now;
        uint32_t hwT = fileWriteTime(HW_CONFIG_PATH);
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
