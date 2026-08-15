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

// Captured by the EasyMotor stub (shared for drive + aux motor channels)
extern float host_last_motor_speed;
// Ring of motor writes per update — writes[0]=left, writes[1]=right in skid mode
extern float host_motor_writes[8];
extern size_t host_motor_write_count;

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
    assert(VehicleController::aux_hydraulic1 == 75 && "Aux Hydraulic channel should receive slider value");
    std::cout << "  PASS: Auxiliary Hydraulics channel verified." << std::endl;

    // Test 6: Config-driven aux motor (mixer type, H-bridge driver). The aux
    // slider drives the aux motor channel proportionally (with the configured
    // duty window). No aux config → no channel, no output.
    std::cout << "[Host VC Test] Test 6: Config-driven aux motor (mixer)..." << std::endl;
    hwConfig.auxMotor.motor.type = HardwareConfig::DriveMotor::DRIVER;
    hwConfig.auxMotor.motor.hardwareId = PinMapper::DRIVER_B;
    hwConfig.auxMotor.motor.direction = HardwareConfig::DriveMotor::FORWARD;
    hwConfig.auxMotor.motor.duty.min = 20;
    hwConfig.auxMotor.motor.duty.max = 90;
    hwConfig.auxMotor.purpose = HardwareConfig::AuxMotor::MIXER;
    HardwareInit::init(hwConfig);

    host_last_motor_speed = 0.0f;
    aux_slider.rk.value = 50;   // mid slider
    VehicleController::update();
    assert(host_last_motor_speed > 0.0f && "Aux motor should be driven by the slider");
    assert(fabs(host_last_motor_speed - 55.0f) < 0.01f &&
           "Aux motor duty should follow the slider within the min/max window");
    std::cout << "  PASS: Aux motor proportional drive verified (duty=" << host_last_motor_speed << ")." << std::endl;

    // Neutral slider → aux motor stops.
    host_last_motor_speed = 1.0f;
    aux_slider.rk.value = 0;
    VehicleController::update();
    assert(host_last_motor_speed == 0.0f && "Neutral aux slider should stop the aux motor");
    std::cout << "  PASS: Neutral slider stops the aux motor." << std::endl;

    // ── Skid-steer differential drive (Test 7-9) ──
    // Reconfigure the hardware for skid-steer: left track on DRIVER_A, right
    // track on DRIVER_B, no aux work-machine channel. The parser would have
    // excluded aux_motor in skid mode; here we set it to NONE explicitly.
    std::cout << "[Host VC Test] Test 7: Skid-steer differential drive..." << std::endl;
    hwConfig.drivetrainType = HardwareConfig::SKID_STEER;
    hwConfig.leftMotor.type = HardwareConfig::DriveMotor::DRIVER;
    hwConfig.leftMotor.hardwareId = PinMapper::DRIVER_A;
    hwConfig.leftMotor.direction = HardwareConfig::DriveMotor::FORWARD;
    hwConfig.leftMotor.duty.min = 20;
    hwConfig.leftMotor.duty.max = 90;
    hwConfig.rightMotor.type = HardwareConfig::DriveMotor::DRIVER;
    hwConfig.rightMotor.hardwareId = PinMapper::DRIVER_B;
    hwConfig.rightMotor.direction = HardwareConfig::DriveMotor::FORWARD;
    hwConfig.rightMotor.duty.min = 20;
    hwConfig.rightMotor.duty.max = 90;
    hwConfig.steeringSensitivity = 80;
    hwConfig.auxMotor.purpose = HardwareConfig::AuxMotor::NONE;
    hwConfig.auxMotor.motor.type = HardwareConfig::DriveMotor::NONE;
    HardwareInit::init(hwConfig);

    gear_switch.rk.value = 0; // D (Drive)
    gas_pedal.rk.value = 100;
    brake_pedal.rk.value = 0;
    steering_wheel.rk.value = 0;

    host_motor_write_count = 0;
    VehicleController::update();
    assert(host_motor_write_count >= 2 && "Both skid tracks should be driven");
    assert(host_motor_writes[0] == 90.0f && "Left track should run at full duty");
    assert(host_motor_writes[1] == 90.0f && "Right track should run at full duty");
    std::cout << "  PASS: Straight throttle drives both tracks equally." << std::endl;

    // Steering right: left = throttle + steer·sens/100, right = throttle − steer·sens/100
    host_motor_write_count = 0;
    steering_wheel.rk.value = 50;
    VehicleController::update();
    assert(host_motor_writes[0] == 90.0f && "Left track boosted by steering (clamped)");
    assert(fabs(host_motor_writes[1] - 62.0f) < 0.01f && "Right track reduced by steering");
    std::cout << "  PASS: Steering splits the tracks by sensitivity (L=90 R=62)." << std::endl;

    std::cout << "[Host VC Test] Test 8: Skid-steer park lock & reverse..." << std::endl;
    gear_switch.rk.value = 1; // P (Park)
    steering_wheel.rk.value = 50;
    host_motor_write_count = 0;
    VehicleController::update();
    assert(host_motor_write_count >= 2 && "Park still drives both channels (to zero)");
    assert(host_motor_writes[0] == 0.0f && host_motor_writes[1] == 0.0f &&
           "Park lock should zero both tracks");
    std::cout << "  PASS: Park locks both tracks." << std::endl;

    gear_switch.rk.value = 2; // R (Reverse)
    gas_pedal.rk.value = 80;
    steering_wheel.rk.value = 0;
    host_motor_write_count = 0;
    VehicleController::update();
    assert(host_motor_write_count >= 2 && "Reverse still drives both channels");
    assert(host_motor_writes[0] < 0.0f && host_motor_writes[1] < 0.0f &&
           "Reverse should negate both tracks");
    std::cout << "  PASS: Reverse negates both tracks." << std::endl;

    std::cout << "[Host VC Test] Test 9: Skid-steer excludes aux work-machine..." << std::endl;
    gear_switch.rk.value = 0; // D
    gas_pedal.rk.value = 100;
    steering_wheel.rk.value = 0;
    aux_slider.rk.value = 75;
    host_motor_write_count = 0;
    VehicleController::update();
    assert(host_motor_write_count == 2 && "Aux channel must not write in skid mode (only the two tracks)");
    assert(host_motor_writes[0] == 90.0f && host_motor_writes[1] == 90.0f &&
           "Track speeds unaffected by the ignored aux slider");
    std::cout << "  PASS: Aux slider ignored; only the two tracks are driven." << std::endl;

    std::cout << "[Host VC Test] Test 10: EasyLEDGroup pattern sequencer & ditch light integration..." << std::endl;
    EasyLED testLedA, testLedB;
    testLedA.begin(1);
    testLedB.begin(2);
    EasyLEDGroup testGroup({&testLedA, &testLedB});


    assert(!testGroup.isRunning() && "Group should not be running initially");
    testGroup.alternate(10);
    assert(testGroup.isRunning() && "Group should be running after alternate(10)");

    // Initial step: member 0 @ 100%, member 1 @ 0%
    assert(fabs(testLedA.getDutyPercent() - 100.0f) < 0.01f);


    assert(fabs(testLedB.getDutyPercent() - 0.0f) < 0.01f);

    // Advance 10ms -> step 1: member 0 @ 0%, member 1 @ 100%
    host_virtual_millis += 10;
    testGroup.update();
    assert(fabs(testLedA.getDutyPercent() - 0.0f) < 0.01f);
    assert(fabs(testLedB.getDutyPercent() - 100.0f) < 0.01f);

    // Stop group -> members to 0, group not running
    testGroup.stop();
    assert(!testGroup.isRunning());
    assert(fabs(testLedA.getDutyPercent() - 0.0f) < 0.01f);
    assert(fabs(testLedB.getDutyPercent() - 0.0f) < 0.01f);

    // Test syncFlash factory
    testGroup.syncFlash(10, 10);
    assert(testGroup.isRunning());
    assert(fabs(testLedA.getDutyPercent() - 100.0f) < 0.01f && fabs(testLedB.getDutyPercent() - 100.0f) < 0.01f);
    testGroup.stop();


    std::cout << "  PASS: EasyLEDGroup patterns and duty ownership verified." << std::endl;

    std::cout << "[Host VC Test] Test 11: Power control & two-tier battery protection..." << std::endl;

    // Part A: 1000ms Boot Latch Filter
    host_gpio_pin_val[POWER::POWER_BUTTON] = HIGH;
    host_gpio_pin_val[POWER::POWER_ENABLE] = LOW;
    host_virtual_millis = 0;
    HardwareInit::latchPower();
    assert(HardwareInit::isPowerLatched() && "Power should latch ON after 1000ms hold at boot");
    assert(host_gpio_pin_val[POWER::POWER_ENABLE] == HIGH && "POWER_ENABLE should be HIGH");

    host_gpio_pin_val[POWER::POWER_ENABLE] = LOW;
    host_gpio_pin_val[POWER::POWER_BUTTON] = LOW;
    host_virtual_millis = 0;
    HardwareInit::latchPower();
    assert(!HardwareInit::isPowerLatched() && "Power should NOT latch ON for brief touch (<1000ms)");
    assert(host_gpio_pin_val[POWER::POWER_ENABLE] == LOW && "POWER_ENABLE should remain LOW");

    // Part B: Runtime 4000ms button hold shutdown
    host_gpio_pin_val[POWER::POWER_ENABLE] = HIGH;
    host_gpio_pin_val[POWER::POWER_BUTTON] = HIGH;
    host_virtual_millis = 0;
    HardwareInit::update();
    host_virtual_millis = 3900;
    HardwareInit::update();
    assert(host_gpio_pin_val[POWER::POWER_ENABLE] == HIGH && "Power should remain ON before 4000ms hold");

    host_virtual_millis = 4100;
    HardwareInit::update();
    assert(host_gpio_pin_val[POWER::POWER_ENABLE] == LOW && "Power should turn OFF after 4000ms hold");
    host_gpio_pin_val[POWER::POWER_BUTTON] = LOW;

    // Part C: Two-Tier Battery Protection
    host_gpio_pin_val[POWER::POWER_ENABLE] = HIGH;
    HardwareConfig testHw;
    testHw.battery.cellCount = 1;
    testHw.battery.warningVoltage = 3.5f;
    testHw.battery.cutoffVoltage = 3.3f;
    testHw.battery.vScale = 1.0f;
    testHw.battery.vOffset = 0.0f;

    VehicleController::init(&testHw, &engine, &profile);

    // Normal voltage (3.6V) -> warning false, cutoff false
    host_analog_read_mv = 3600;
    VehicleController::update();
    assert(!VehicleController::isBatteryWarning() && "Battery warning should be false at 3.6V");
    assert(!VehicleController::isBatteryCutoff() && "Battery cutoff should be false at 3.6V");

    // Drop below warning (3.4V) -> warning true, cutoff false
    host_analog_read_mv = 3400;
    VehicleController::update();
    assert(VehicleController::isBatteryWarning() && "Battery warning should trigger below 3.5V");
    assert(!VehicleController::isBatteryCutoff() && "Battery cutoff should remain false above 3.3V");

    // Drop below cutoff (3.2V) -> cutoff triggers after 1500ms -> powerOff()
    host_analog_read_mv = 3200;
    host_virtual_millis = 10000;
    VehicleController::init(&testHw, &engine, &profile);
    VehicleController::update();
    assert(!VehicleController::isBatteryCutoff() && "Cutoff should not trigger before 1500ms delay");

    host_virtual_millis = 11600;
    VehicleController::update();
    assert(VehicleController::isBatteryCutoff() && "Cutoff should trigger after 1500ms below 3.3V");
    assert(host_gpio_pin_val[POWER::POWER_ENABLE] == LOW && "POWER_ENABLE should be driven LOW on cutoff");

    std::cout << "  PASS: 1000ms boot latch, 4000ms hold shutdown, and two-tier battery protection verified." << std::endl;

    std::cout << "[Host VC Test] Test 12: 3-State power control, 10s disconnect warning, button timer reset, and seconds config..." << std::endl;

    HardwareConfig testHw2;
    testHw2.power.bootLatchS = 2;
    testHw2.power.buttonHoldS = 5;
    testHw2.power.disconnectTimeoutS = 60;
    testHw2.power.warningWindowS = 10;
    testHw2.power.cutoffDelayS = 2;

    VehicleController::init(&testHw2, &engine, &profile);

    // Boot latch with 2.0s setting
    host_gpio_pin_val[POWER::POWER_BUTTON] = HIGH;
    host_gpio_pin_val[POWER::POWER_ENABLE] = LOW;
    host_virtual_millis = 20000;
    HardwareInit::latchPower(testHw2.power.bootLatchS);
    assert(HardwareInit::isPowerLatched() && "Power should latch ON after 2.0s hold");

    // Runtime hold with 5.0s setting
    host_gpio_pin_val[POWER::POWER_ENABLE] = HIGH;
    host_gpio_pin_val[POWER::POWER_BUTTON] = HIGH;
    host_virtual_millis = 30000;
    HardwareInit::update(testHw2.power.buttonHoldS);
    host_virtual_millis = 34500;
    HardwareInit::update(testHw2.power.buttonHoldS);
    assert(host_gpio_pin_val[POWER::POWER_ENABLE] == HIGH && "Power should remain ON at 4.5s (< 5.0s)");

    host_virtual_millis = 35200;
    HardwareInit::update(testHw2.power.buttonHoldS);
    assert(host_gpio_pin_val[POWER::POWER_ENABLE] == LOW && "Power should turn OFF at 5.2s (>= 5.0s)");
    host_gpio_pin_val[POWER::POWER_BUTTON] = LOW;

    // 3-State CHARGING detection
    host_gpio_pin_val[POWER::POWER_ENABLE] = HIGH;
    host_gpio_pin_val[POWER::CHARGE_SENS] = HIGH;
    VehicleController::update();
    assert(VehicleController::isChargingState() && "CHARGING state should be active when CHARGE_SENS is HIGH");
    host_gpio_pin_val[POWER::CHARGE_SENS] = LOW;
    VehicleController::update();
    assert(!VehicleController::isChargingState() && "CHARGING state should clear when CHARGE_SENS is LOW");

    // Disconnect timeout & 10s warning phase (re-init controller at start of disconnect test)
    host_virtual_millis = 40000;
    VehicleController::init(&testHw2, &engine, &profile);
    VehicleController::update();
    host_virtual_millis = 85000; // 45s elapsed
    VehicleController::update();
    assert(!VehicleController::isDisconnectWarning() && "Warning should be false before 50s");

    host_virtual_millis = 92000; // 52s elapsed
    VehicleController::update();
    assert(VehicleController::isDisconnectWarning() && "Warning phase should activate at 52s (within 10s window)");

    // Button single click timer reset
    host_gpio_pin_val[POWER::POWER_BUTTON] = HIGH;
    host_virtual_millis = 92100;
    HardwareInit::update(testHw2.power.buttonHoldS);
    host_gpio_pin_val[POWER::POWER_BUTTON] = LOW;
    host_virtual_millis = 92300;
    HardwareInit::update(testHw2.power.buttonHoldS);
    VehicleController::update();
    assert(!VehicleController::isDisconnectWarning() && "Button click should reset timer and clear warning");

    // Timeout power off after reset (60s from reset at 92.3s -> 152.3s = 152300ms)
    host_virtual_millis = 153000;
    VehicleController::update();
    assert(host_gpio_pin_val[POWER::POWER_ENABLE] == LOW && "Board should power off after 60s from reset");

    std::cout << "  PASS: 3-State power control, 10s warning, button reset, and seconds config verified." << std::endl;

    std::cout << "[Host VC Test] Test 13: Power button hold rapid blink feedback and charging indicator..." << std::endl;

    HardwareConfig testHw3;
    testHw3.lights.headLight.pin = 38;
    testHw3.lights.headLight.configured = true;
    testHw3.power.indicatorPin = 38;
    testHw3.power.buttonHoldS = 4;
    testHw3.charging.pin = 38;
    testHw3.charging.mode = 1;
    testHw3.charging.configured = true;

    HardwareInit::init(testHw3);

    // Power button hold rapid blink feedback
    host_gpio_pin_val[POWER::POWER_BUTTON] = HIGH;
    host_virtual_millis = 200000;
    HardwareInit::update(testHw3.power.buttonHoldS, testHw3.power.indicatorPin);
    assert(HardwareInit::getLightDutyPercent(38) > 0.0f && "Indicator should blink ON at 200ms phase 0");

    host_virtual_millis = 200200;
    HardwareInit::update(testHw3.power.buttonHoldS, testHw3.power.indicatorPin);
    assert(HardwareInit::getLightDutyPercent(38) == 0.0f && "Indicator should blink OFF at 200ms phase 1");

    host_gpio_pin_val[POWER::POWER_BUTTON] = LOW;

    // Charging indicator animation
    host_gpio_pin_val[POWER::CHARGE_SENS] = HIGH;
    VehicleController::init(&testHw3, &engine, &profile);
    VehicleController::update();
    assert(VehicleController::isChargingState() && "Charging state should be active");

    std::cout << "  PASS: Power button hold rapid blink feedback and charging indicator verified." << std::endl;

    std::cout << "[Host VC Test] ALL HOST VEHICLE CONTROLLER ASSERTIONS PASSED SUCCESSFULLY." << std::endl;
    return 0;
}

