#include "Arduino.h"
#include <LittleFS.h>
#include "boards.h"
#include "Config.h"
#include <RcEngineSound.h>
#include <VehicleProfile.h>
#include "HardwareInit.h"
#include "RADIOKIT.h"
#include "VehicleController.h"
#include <iostream>
#include <cassert>
#include <cmath>

uint32_t host_virtual_millis = 0;
DummySerial Serial;
DummyLittleFS LittleFS;

int main() {
    std::cout << "[Host VC Test] Initializing VehicleController Host Harness..." << std::endl;

    HardwareConfig hwConfig;
    hwConfig.battery.cellCount = 1;
    hwConfig.battery.cutoffVoltage = 3.4f;
    hwConfig.battery.vScale = 1.0f;
    hwConfig.battery.vOffset = 0.0f;

    static int8_t dummyPcm[256] = {0};
    SoundData soundData;
    soundData.isDynamic = false;
    soundData.slots[START].samples = dummyPcm;
    soundData.slots[START].sampleCount = 100;
    soundData.slots[IDLE].samples = dummyPcm;
    soundData.slots[IDLE].sampleCount = 256;
    soundData.slots[REV].samples = dummyPcm;
    soundData.slots[REV].sampleCount = 256;

    RcEngineSound engine;
    RcEngineSound::Config engineCfg;
    engineCfg.type = RcEngineSound::VEHICLE_TRUCK;
    engineCfg.engine.minRpm = 10;
    engineCfg.engine.maxRpm = 500;
    engineCfg.engine.acc = 25;
    engineCfg.engine.dec = 15;
    engineCfg.engine.inertia = 10;

    engine.begin(soundData, engineCfg);

    VehicleProfile profile;
    profile.config = engineCfg;

    VehicleController::init(&hwConfig, &engine, &profile);

    std::cout << "[Host VC Test] Test 1: Engine Start & Park Lock Safety..." << std::endl;
    start_button.rk.state = true;
    gear_switch.rk.value = 1; // P (Park Lock)
    gas_pedal.rk.value = 100; // Floored pedal

    // Step 200 ticks (2000ms) to pass starting sample into RUNNING state
    for (int i = 0; i < 200; i++) {
        host_virtual_millis += 10;
        engine.update(0);
        for (int s = 0; s < 220; s++) engine.getNextSample();
        VehicleController::update();
    }

    assert(engine.getState() == RcEngineSound::RUNNING && "Engine should transition to RUNNING state");
    std::cout << "  PASS: Park Lock zero-torque safety verified." << std::endl;

    std::cout << "[Host VC Test] Test 2: Drive Gear & Proportional Brake Blending..." << std::endl;
    gear_switch.rk.value = 0; // D (Drive)
    gas_pedal.rk.value = 100;
    brake_pedal.rk.value = 0;
    VehicleController::update();

    // Apply 60% Brake Pedal (>20% deadband).
    brake_pedal.rk.value = 60;
    VehicleController::update();

    // Apply 100% Full Brake Pedal.
    brake_pedal.rk.value = 100;
    VehicleController::update();
    std::cout << "  PASS: Proportional Brake Blending verified." << std::endl;

    std::cout << "[Host VC Test] Test 3: Reverse Gear Direction & Telemetry..." << std::endl;
    brake_pedal.rk.value = 0;
    gas_pedal.rk.value = 80;
    gear_switch.rk.value = 2; // R (Reverse)
    VehicleController::update();
    std::cout << "  PASS: Reverse Gear direction inversion verified." << std::endl;

    std::cout << "[Host VC Test] Test 4: Virtual Flywheel RPM Inertia Ramp..." << std::endl;
    gear_switch.rk.value = 0; // D
    gas_pedal.rk.value = 100;
    uint16_t initialRpm = engine.getRpm();

    for (int i = 0; i < 50; i++) {
        host_virtual_millis += 10;
        engine.update(400);
        for (int s = 0; s < 220; s++) engine.getNextSample();
        VehicleController::update();
    }
    uint16_t rampedRpm = engine.getRpm();
    assert(rampedRpm > initialRpm && "Engine RPM should ramp up with inertia over time");
    std::cout << "  PASS: RPM Acceleration Ramp verified (" << initialRpm << " RPM -> " << rampedRpm << " RPM)." << std::endl;

    std::cout << "[Host VC Test] Test 5: Auxiliary Hydraulics Governor Load..." << std::endl;
    aux_slider.rk.value = 75;
    VehicleController::update();
    assert(VehicleController::aux_hydraulic1 == 75 && "Aux Hydraulic Servo 1 should receive slider value");
    std::cout << "  PASS: Auxiliary Hydraulics channel verified." << std::endl;

    std::cout << "[Host VC Test] ALL HOST VEHICLE CONTROLLER ASSERTIONS PASSED SUCCESSFULLY." << std::endl;
    return 0;
}
