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

RcEngineSound engine;
VehicleProfile profile;
HardwareConfig hwConfig;

// ── Config hot-reload & Control task state ──
static TaskHandle_t s_controlTaskHandle = nullptr;
static volatile bool s_configReloadPending = false;
static bool s_operationalMode = true;

static void startOperationalStack();
static void stopOperationalStackToCharging();

static void controlTask(void* param) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(20); // Strict 50 Hz (20.0 ms) period

    while (true) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        HardwareInit::update(hwConfig.power.buttonHoldS, hwConfig.power.indicatorPin);

        if (Serial.available()) {
            char c = Serial.read();
            if (c == 'w' || c == 'W' || c == '\n') {
                if (!s_operationalMode) {
                    Serial.println("[MAIN] Serial wake command received -> Switching ON");
                    startOperationalStack();
                }
            }
        }

        static uint64_t s_lastPinMask = 0;
        uint64_t curMask = 0;
        const int probePins[] = {0, 1, 2, 3, 4, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 21, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 45, 46, 47, 48};
        for (int p : probePins) {
            if (digitalRead(p)) curMask |= (1ULL << p);
        }
        if (curMask != s_lastPinMask) {
            for (int p : probePins) {
                if (((curMask ^ s_lastPinMask) >> p) & 1) {
                    Serial.printf("[PIN_EVENT] GPIO %d changed to %d\n", p, (int)((curMask >> p) & 1));
                }
            }
            s_lastPinMask = curMask;
        }

        if (HardwareInit::isButtonClicked()) {
            if (!s_operationalMode) {
                Serial.println("[MAIN] Power button clicked in charging mode -> Switching ON");
                startOperationalStack();
            }
        }

        if (HardwareInit::isPowerOffRequested()) {
            HardwareInit::clearPowerOffRequested();
            if (HardwareInit::isUsbPowered() || HardwareInit::isCharging()) {
                Serial.println("[MAIN] Power-off requested while USB connected -> Switching to Charging Mode");
                stopOperationalStackToCharging();
            } else {
                Serial.println("[MAIN] Power-off requested on battery -> Powering off");
                HardwareInit::powerOff();
            }
        }

        VehicleController::update();

        HardwareInit::clearButtonClicked();
    }
}

// Aux slider control profile from aux_motor.type. mixer: 5 detents, no
// self-centering (the user sets a speed/direction that keeps running); tipper:
// no detents, self-centering (momentary — follows the finger). trailer_dcc
// leaves the default (no aux channel), matching the deferred state.
// Must run after config load and before the app connects (RadioKit sends the
// widget config on connect).
static void applyAuxSliderProfile() {
    if (hwConfig.auxMotorCount > 0) {
        if (hwConfig.auxMotors[0].purpose == HardwareConfig::AuxMotor::MIXER) {
            aux_slider.rk.centering = RK_SPRING_NONE;
            aux_slider.rk.detents = 5;
        } else if (hwConfig.auxMotors[0].purpose == HardwareConfig::AuxMotor::TIPPER) {
            aux_slider.rk.centering = RK_SPRING_CENTER;
            aux_slider.rk.detents = 0;
        }
    }
}

#if defined(RK_ENABLE_BLE)
#include <NimBLEDevice.h>
#endif

#ifdef MIKRO_V2
#define BOARD_DEFAULT_NAME "MIKRO_V2"
#elif defined(TRACKLINK_V2)
#define BOARD_DEFAULT_NAME "TRACKLINK_V2"
#elif defined(TRACKLINK_V3)
#define BOARD_DEFAULT_NAME "TRACKLINK_V3"
#elif defined(GTRACK)
#define BOARD_DEFAULT_NAME "GTRACK"
#elif defined(RK_ULTRA)
#define BOARD_DEFAULT_NAME "RK_ULTRA"
#else
#define BOARD_DEFAULT_NAME "RC_Engine"
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
        RadioKit.config.name = BOARD_DEFAULT_NAME;
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

    RadioKit.setConfig(RadioKit.config.name, RadioKit.config.description);

#if defined(RK_ENABLE_BLE)
    NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
    if (pAdv && pAdv->isAdvertising()) {
        pAdv->stop();
        NimBLEAdvertisementData scanData;
        // Prefix "RK_" so the app scan filter (startsWith('RK_')) matches.
        static char bleName[RADIOKIT_MAX_NAME + 4];
        snprintf(bleName, sizeof(bleName), "RK_%s", RadioKit.config.name);
        scanData.setName(bleName);
        pAdv->setScanResponseData(scanData);
        pAdv->start();
    }
#endif

    Serial.printf("[Device] Name: '%s', Description: '%s', Type: '%s'\n",
                  RadioKit.config.name, RadioKit.config.description, RadioKit.config.type);
}

static bool s_audioStarted = false;
static bool s_hasSoundAssets = false;
static bool s_radioKitStarted = false;

static void startOperationalStack() {
    if (s_operationalMode && s_radioKitStarted) return;
    Serial.println("\n── Starting RadioKit & Audio Stack (Switched ON) ──");
    HardwareInit::clearUsbBoot();

    if (!s_radioKitStarted) {
        initRadioKit();
        RadioKit.config.name = BOARD_DEFAULT_NAME;
        RadioKit.config.description = "RC Engine Controller";
        applyDeviceMetadata(hwConfig, profile.config);
        applyAuxSliderProfile();
        UiLogger::onRadioKitStarted();
        s_radioKitStarted = true;

        rk_fsSetCallback([](uint8_t subCmd, const uint8_t* payload, uint16_t payloadLen) {
            if (subCmd == RK_FS_CMD_UPLOAD_END || subCmd == RK_FS_CMD_REPLACE || subCmd == RK_FS_CMD_WRITE) {
                Serial.println("[RadioKit] FS write/upload complete -> scheduling config reload");
                s_configReloadPending = true;
            }
        });
    } else {
#if defined(RK_ENABLE_BLE)
        NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
        if (pAdv && !pAdv->isAdvertising()) {
            pAdv->start();
        }
#endif
    }

    if (hwConfig.sound.configured && s_hasSoundAssets && !s_audioStarted) {
        Serial.println("\n── Starting Engine Simulation & Audio Output ──");
        engine.setConfig(profile.config);
        engine.begin(profile.sounds, profile.config);
        AudioOutput::begin(&engine);
        AudioOutput::start();
        s_audioStarted = true;
    }

    s_operationalMode = true;
}

static void stopOperationalStackToCharging() {
    if (!s_operationalMode) return;
    Serial.println("\n── Switching to Minimal Charging Mode (USB Connected) ──");

    if (s_audioStarted) {
        AudioOutput::stop();
        s_audioStarted = false;
    }
    if (engine.getState() != RcEngineSound::OFF) {
        engine.stopEngine();
    }

#if defined(RK_ENABLE_BLE)
    NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
    if (pAdv && pAdv->isAdvertising()) {
        pAdv->stop();
    }
    NimBLEServer* pServer = NimBLEDevice::getServer();
    if (pServer) {
        auto peers = pServer->getPeerDevices();
        for (auto& peer : peers) {
            pServer->disconnect(peer);
        }
    }
#endif

    HardwareInit::stopAll();
    HardwareInit::setUsbBoot(true);
    s_operationalMode = false;

    if (Board::isValid(POWER::POWER_ENABLE)) {
        uint8_t gpio = Board::toGpio(POWER::POWER_ENABLE);
        bool inv = Board::isPinInverted(POWER::POWER_ENABLE);
        digitalWrite(gpio, inv ? LOW : HIGH);
    }
}

static bool reloadConfigs() {
    Serial.println("\n── Reloading Configs ──");
    UiLogger::clear();

    HardwareConfig newHw;
    RcEngineSound::Config newVc;

    String hwPath = ConfigParser::findHardwareConfig();
    String vcPath = ConfigParser::findVehicleConfig();

    if (hwPath.length() == 0 || !ConfigParser::loadHardwareConfig(hwPath.c_str(), newHw)) {
        Serial.println("[Reload] No valid hardware-*.json found — keeping current config");
        UiLogger::logf("ERR: hardware config invalid");
        return false;
    }
    if (vcPath.length() == 0 || !ConfigParser::loadVehicleConfig(vcPath.c_str(), newVc)) {
        Serial.println("[Reload] No valid vehicle config found — keeping current config");
        UiLogger::logf("ERR: vehicle config invalid");
        return false;
    }

    bool wasConfigured = (hwConfig.name[0] != '\0' || profile.config.name[0] != '\0');
    bool nameChanged = strcmp(newVc.name, profile.config.name) != 0;

    hwConfig = newHw;
    profile.config = newVc;
    profile.config.sound.master = hwConfig.sound.configured ? hwConfig.sound.volume : 0;

    HardwareInit::hotReload(hwConfig);
    VehicleController::init(&hwConfig, &engine, &profile);
    VehicleController::applyConfiguredLightMask(hwConfig.lights, hwConfig.auxLight.configured);
    engine.setConfig(profile.config);
    applyAuxSliderProfile();
    applyDeviceMetadata(hwConfig, profile.config);

    bool hasSoundHardware = Board::hasAudio() && hwConfig.sound.configured;
    bool hasSoundAssets = false;

    if (hasSoundHardware && (nameChanged || !wasConfigured)) {
        hasSoundAssets = ConfigParser::loadSounds(profile.config, profile.sounds);
    }

    if (nameChanged || !wasConfigured) {
        engine.begin(profile.sounds, profile.config);
    }

    if (hasSoundHardware && (hasSoundAssets || s_audioStarted)) {
        if (!s_audioStarted) {
            AudioOutput::begin(&engine);
            AudioOutput::start();
            s_audioStarted = true;
        }
    } else if (!hasSoundHardware && s_audioStarted) {
        AudioOutput::stop();
        s_audioStarted = false;
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
    if (!Board::hasAudio()) {
        Serial.println("  Sound: DISABLED (no audio hardware on board)");
    } else if (hw.sound.configured) {
        Serial.printf("  Sound: ENABLED (Volume: %d%%)\n", hw.sound.volume);
    } else {
        Serial.println("  Sound: DISABLED (not defined in hardware config)");
    }
    for (uint8_t i = 0; i < hw.driveMotorCount; i++) {
        Serial.printf("  Drive Motor[%d]: type=%d hwId=%d freq=%dHz dir=%d\n",
                      i, hw.driveMotors[i].type, hw.driveMotors[i].hardwareId,
                      hw.driveMotors[i].frequency, hw.driveMotors[i].direction);
    }
    for (uint8_t i = 0; i < hw.steeringServoCount; i++) {
        Serial.printf("  Steering Servo[%d]: hwId=%d freq=%dHz L=%d R=%d C=%d\n",
                      i, hw.steeringServos[i].hardwareId, hw.steeringServos[i].frequency,
                      hw.steeringServos[i].endpoints.left, hw.steeringServos[i].endpoints.right,
                      hw.steeringServos[i].endpoints.center);
    }
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

    Serial.println("\n=== RC Engine - Unified Vehicle Controller ===\n");

    if (!ConfigParser::begin()) {
        Serial.println("FATAL: LittleFS mount failed");
        UiLogger::log("FATAL: LittleFS mount failed");
    }

    ConfigParser::printFilesystemInfo();

    // Check if booted via USB (minimal charging mode)
    if (HardwareInit::isUsbBoot()) {
        s_operationalMode = false;
        Serial.println("\n── USB Boot: Minimal Charging Mode (Modem & Audio OFF) ──");
        Serial.println("  [Tip] Press the power button to switch ON and begin driving.");
    } else {
        s_operationalMode = true;
    }

    Serial.println("\n── Loading Configs ──");
    String hwPath = ConfigParser::findHardwareConfig();
    String vcPath = ConfigParser::findVehicleConfig();

    bool hwOk = (hwPath.length() > 0) && ConfigParser::loadHardwareConfig(hwPath.c_str(), hwConfig);
    bool vcOk = (vcPath.length() > 0) && ConfigParser::loadVehicleConfig(vcPath.c_str(), profile.config);

    if (hwOk && vcOk) {
        Serial.printf("[Config] Loaded HW: %s, Vehicle: %s\n", hwPath.c_str(), vcPath.c_str());
        printConfig(hwConfig, profile.config);

        if (hwConfig.sound.configured) {
            Serial.println("\n── Loading Sounds ──");
            s_hasSoundAssets = ConfigParser::loadSounds(profile.config, profile.sounds);
        } else {
            Serial.println("\n── Audio Hardware: DISABLED (omitted from hardware config) ──");
        }

        Serial.println("\n── Initializing Hardware ──");
        HardwareInit::init(hwConfig);

        Serial.println("\n── Initializing Vehicle Controller ──");
        profile.config.sound.master = hwConfig.sound.configured ? hwConfig.sound.volume : 0;
        VehicleController::init(&hwConfig, &engine, &profile);

        if (s_operationalMode) {
            startOperationalStack();
        }

        VehicleController::applyConfiguredLightMask(hwConfig.lights, hwConfig.auxLight.configured);
    } else {
        Serial.println("\n── Recovery Mode Active ──");
        Serial.printf("Config missing or incomplete (hardware-*.json [%s]: %s, vehicle-*.json [%s]: %s)\n",
                      hwPath.c_str(), hwOk ? "OK" : "MISSING",
                      vcPath.c_str(), vcOk ? "OK" : "MISSING");
        Serial.println("RadioKit BLE and LittleFS are active. Upload configuration files to initialize.");
        UiLogger::log("WARN: Config missing — Recovery Mode");
        startOperationalStack();
    }

    // Start deterministic 50 Hz control task (Priority 2, Core 1)
    xTaskCreatePinnedToCore(controlTask, "control", 8192, nullptr, 2, &s_controlTaskHandle, 1);

    Serial.println("\n── System Ready ──");
    Serial.printf("CPU Frequency: %d MHz\n", getCpuFrequencyMhz());
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
    Serial.println("\n=== Init Complete ===\n");
}

void loop() {
    if (s_operationalMode) {
        RadioKit.update();
    } else {
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    // Event-driven configuration reload triggered upon upload completion
    if (s_configReloadPending) {
        s_configReloadPending = false;
        reloadConfigs();
    }
}
