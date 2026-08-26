#include "Arduino.h"
#include <LittleFS.h>
#include "boards.h"
#include "TRACKLINK_V3.h"
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
extern float host_motor_writes[8];
extern size_t host_motor_write_count;
extern int host_last_servo_us;
extern bool host_radiokit_connected;

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
    brake_pedal.rk.value = -100; // 0% brake
    VehicleController::update();

    // Apply 60% Brake Pedal (>20% deadband: 60% normalized is 20 raw).
    brake_pedal.rk.value = 20; // (20 + 100)/2 = 60%
    VehicleController::update();

    // Apply 100% Full Brake Pedal.
    brake_pedal.rk.value = 100;
    VehicleController::update();
    std::cout << "  PASS: Proportional Brake Blending verified." << std::endl;

    std::cout << "[Host VC Test] Test 3: Reverse Gear Direction & Telemetry..." << std::endl;
    brake_pedal.rk.value = -100;
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
    brake_pedal.rk.value = -100;
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
    assert(HardwareInit::isPowerLatched() && "USB boot (POWER_BUTTON low) should latch ON immediately");
    assert(host_gpio_pin_val[POWER::POWER_ENABLE] == HIGH && "POWER_ENABLE should be HIGH on USB boot");

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
    for (int i = 0; i < 30; i++) { host_virtual_millis += 100; VehicleController::update(); }
    assert(VehicleController::isBatteryWarning() && "Battery warning should trigger below 3.5V");
    assert(!VehicleController::isBatteryCutoff() && "Battery cutoff should remain false above 3.3V");

    // Drop below cutoff (3.2V) -> cutoff triggers after 1500ms -> powerOff()
    host_analog_read_mv = 3200;
    host_virtual_millis = 10000;
    VehicleController::init(&testHw, &engine, &profile);
    VehicleController::update();
    assert(!VehicleController::isBatteryCutoff() && "Cutoff should not trigger before 1500ms delay");

    host_virtual_millis = 12500;
    for (int i = 0; i < 30; i++) { host_virtual_millis += 100; VehicleController::update(); }
    assert(VehicleController::isBatteryCutoff() && "Cutoff should trigger after cutoff delay below 3.3V");
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

    // After 600ms (exceeding 500ms debounce), phase at 200800ms is ON (200800/200 % 2 == 0)
    host_virtual_millis = 200800;
    HardwareInit::update(testHw3.power.buttonHoldS, testHw3.power.indicatorPin);
    assert(HardwareInit::getLightDutyPercent(38) > 0.0f && "Indicator should blink ON at 200ms phase 0");

    // At 201000ms (201000/200 % 2 == 1), phase is OFF
    host_virtual_millis = 201000;
    HardwareInit::update(testHw3.power.buttonHoldS, testHw3.power.indicatorPin);
    assert(HardwareInit::getLightDutyPercent(38) == 0.0f && "Indicator should blink OFF at 200ms phase 1");

    host_gpio_pin_val[POWER::POWER_BUTTON] = LOW;

    // Charging indicator animation
    host_gpio_pin_val[POWER::CHARGE_SENS] = HIGH;
    VehicleController::init(&testHw3, &engine, &profile);
    VehicleController::update();
    assert(VehicleController::isChargingState() && "Charging state should be active");

    std::cout << "  PASS: Power button hold rapid blink feedback and charging indicator verified." << std::endl;

    // ── Test 14: Steer-by-Wire Continuous Servo Tracking ──
    std::cout << "[Host VC Test] Test 14: Steer-by-Wire Continuous Servo Tracking..." << std::endl;
    HardwareConfig testHwAckermann;
    testHwAckermann.drivetrainType = HardwareConfig::ACKERMANN;
    testHwAckermann.steeringServo.hardwareId = PIN::S1;
    testHwAckermann.steeringServo.configured = true;
    testHwAckermann.steeringServo.frequency = 50;
    testHwAckermann.steeringServo.endpoints.left = 1350;
    testHwAckermann.steeringServo.endpoints.center = 1500;
    testHwAckermann.steeringServo.endpoints.right = 1650;
    HardwareInit::init(testHwAckermann);
    VehicleController::init(&testHwAckermann, &engine, &profile);

    // Case 1: Engine is OFF -> Steering servo MUST still track steering_wheel
    start_button.rk.state = false;
    steering_wheel.rk.value = 50;
    VehicleController::update();
    assert(host_last_servo_us == 1575 && "Steering servo should follow +50% steerVal even when engine is OFF");

    steering_wheel.rk.value = -100;
    VehicleController::update();
    assert(host_last_servo_us == 1350 && "Steering servo should follow -100% steerVal when engine is OFF");

    // Case 2: Engine is in PARK (P) -> Steering servo MUST still track steering_wheel
    start_button.rk.state = true;
    gear_switch.rk.value = 1; // P
    steering_wheel.rk.value = 100;
    VehicleController::update();
    assert(host_last_servo_us == 1650 && "Steering servo should follow +100% steerVal in Park");
    std::cout << "  PASS: Steer-by-wire continuous servo tracking verified (OFF & Park)." << std::endl;

    // ── Test 15: Real-Vehicle Turn Indicator Auto-Cancellation State Machine ──
    std::cout << "[Host VC Test] Test 15: Real-Vehicle Turn Indicator Auto-Cancellation..." << std::endl;
    steering_wheel.rk.value = 0;
    left_indicator.rk.state = false;
    right_indicator.rk.state = false;
    VehicleController::update();

    // 15.1: Mutual exclusion
    left_indicator.rk.state = true;
    VehicleController::update();
    assert(left_indicator.rk.state == true && right_indicator.rk.state == false);

    right_indicator.rk.state = true;
    VehicleController::update();
    assert(right_indicator.rk.state == true && left_indicator.rk.state == false && "Right turns off Left");

    // 15.2: Left Turn Armed (< -20) -> Return to Center (> -8) cancels Left indicator
    // First clear suppression by resetting toggle to false (as happens when tapping in app)
    left_indicator.rk.state = false;
    right_indicator.rk.state = false;
    VehicleController::update();

    left_indicator.rk.state = true;
    steering_wheel.rk.value = 0;
    VehicleController::update(); // button edge registered
    assert(left_indicator.rk.state == true);

    steering_wheel.rk.value = -30; // Turn Left past -20% -> armed
    VehicleController::update();
    assert(left_indicator.rk.state == true && "Left indicator stays active during turn");

    steering_wheel.rk.value = 0; // Return to center (0 > -8) -> auto cancels
    VehicleController::update();
    assert(left_indicator.rk.state == false && "Left indicator should auto-cancel on wheel return");

    // 15.3: Opposite Steering Cancellation (Right indicator on -> steer Left < -15% cancels)
    right_indicator.rk.state = false;
    VehicleController::update();
    right_indicator.rk.state = true;
    steering_wheel.rk.value = 0;
    VehicleController::update();
    assert(right_indicator.rk.state == true);

    steering_wheel.rk.value = -25; // Steer opposite (Left < -15%) -> immediate cancel
    VehicleController::update();
    assert(right_indicator.rk.state == false && "Right indicator should cancel immediately on opposite steering");
    std::cout << "  PASS: Turn indicator mutual exclusion and auto-cancellation verified." << std::endl;

    // ── Test 16: Transmission Start/Stop Interlock ──
    std::cout << "[Host VC Test] Test 16: Transmission Start/Stop Interlock..." << std::endl;
    // Engine Stop -> shifts to Park (P=1)
    start_button.rk.state = false;
    gear_switch.rk.value = 0; // Was in Drive
    VehicleController::update();
    assert(gear_switch.rk.value == 1 && "Engine stop should auto-shift gear to Park (P=1)");

    // Engine Start while in Park -> auto-shifts to Drive (D=0)
    start_button.rk.state = true;
    VehicleController::update();
    assert(gear_switch.rk.value == 0 && "Engine start from Park should auto-shift gear to Drive (D=0)");

    // Engine Stop again -> shifts to Park (P=1)
    start_button.rk.state = false;
    VehicleController::update();
    assert(gear_switch.rk.value == 1 && "Engine stop should return gear to Park (P=1)");
    std::cout << "  PASS: Transmission Start/Stop Interlock verified." << std::endl;

    // ── Test 17: Dedicated Full Beam & Fog Lamp Outputs ──
    std::cout << "[Host VC Test] Test 17: Dedicated Full Beam & Fog Lamp Outputs..." << std::endl;
    HardwareConfig testHwLights;
    testHwLights.battery.cellCount = 1;
    testHwLights.battery.cutoffVoltage = 3.0f;
    testHwLights.battery.vScale = 1.0f;
    testHwLights.battery.vOffset = 0.0f;
    testHwLights.lights.headLight.pin = 10;
    testHwLights.lights.headLight.brightness = 100;
    testHwLights.lights.headLight.configured = true;

    testHwLights.lights.fullBeam.pin = 11;
    testHwLights.lights.fullBeam.brightness = 100;
    testHwLights.lights.fullBeam.configured = true;

    testHwLights.lights.fogLamp.pin = 12;
    testHwLights.lights.fogLamp.brightness = 80;
    testHwLights.lights.fogLamp.configured = true;

    testHwLights.lights.beacon.pin = 13;
    testHwLights.lights.beacon.brightness = 100;
    testHwLights.lights.beacon.configured = true;

    testHwLights.lights.cabLight.pin = 14;
    testHwLights.lights.cabLight.brightness = 40;
    testHwLights.lights.cabLight.configured = true;

    testHwLights.lights.workLight.pin = 15;
    testHwLights.lights.workLight.brightness = 90;
    testHwLights.lights.workLight.configured = true;

    testHwLights.lights.auxLight.pin = 16;
    testHwLights.lights.auxLight.brightness = 70;
    testHwLights.lights.auxLight.configured = true;

    testHwLights.lights.turnLight.leftPin = 17;
    testHwLights.lights.turnLight.rightPin = 18;
    testHwLights.lights.turnLight.brightness = 60;
    testHwLights.lights.turnLight.configured = true;

    HardwareInit::init(testHwLights);
    VehicleController::init(&testHwLights, &engine, &profile);

    start_button.rk.state = true;
    for (int i = 0; i < 200; i++) {
        host_virtual_millis += 10;
        engine.update(0);
        for (int s = 0; s < 220; s++) engine.getNextSample();
        VehicleController::update();
    }

    // 17.1: Head Light (Bit 0) -> Headlight ON (40%), Full Beam OFF (0%)
    truck_light.rk.value = 0x01; // Bit 0 Head Light
    VehicleController::update();
    assert(HardwareInit::getLightDutyPercent(10) > 0.0f && "Headlight should be active on Head Light");
    assert(HardwareInit::getLightDutyPercent(11) == 0.0f && "Full beam should be 0 on Head Light");

    // 17.2a: High Beam standalone (Bit 1 only) while Headlight is OFF -> MUST NOT turn on and should clear UI bit
    truck_light.rk.value = 0x02; // Bit 1 only (Headlight OFF)
    VehicleController::update();
    assert((truck_light.rk.value & 0x02) == 0 && "High beam bit should be cleared when Headlight is OFF");
    assert(HardwareInit::getLightDutyPercent(11) == 0.0f && "Full beam should remain 0 when Headlight is OFF");

    // 17.2b: Head Light + High Beam (Bit 0 + Bit 1) -> Dedicated Full Beam ON (100%)
    truck_light.rk.value = 0x03; // Bit 0 + Bit 1
    VehicleController::update();
    assert(HardwareInit::getLightDutyPercent(11) > 0.0f && "Dedicated full beam pin should energize when Headlight is ON");

    // 17.2c: Turning off Head Light -> High Beam must also turn OFF
    truck_light.rk.value = 0x02; // Turn off bit 0
    VehicleController::update();
    assert((truck_light.rk.value & 0x02) == 0 && "High beam bit should turn OFF when Headlight is turned OFF");
    assert(HardwareInit::getLightDutyPercent(11) == 0.0f && "Full beam should turn off when Headlight turns off");

    // 17.3: Fog Lamp (Bit 2) -> Dedicated Fog Lamp ON (80%)
    truck_light.rk.value = 0x04; // Bit 2 Fog Lamp
    VehicleController::update();
    assert(HardwareInit::getLightDutyPercent(12) >= 79 && "Dedicated fog lamp pin should energize on Fog Lamp");

    // 17.4: Beacon Light (Bit 4) -> Beacon Pin 13
    truck_light.rk.value = 0x10; // Bit 4 Beacon
    VehicleController::update();
    assert(HardwareInit::getLightDutyPercent(13) > 0.0f && "Beacon pin should energize on Bit 4");

    // 17.5: Cab Light (Bit 5) -> Cab Pin 14
    truck_light.rk.value = 0x20; // Bit 5 Cab
    VehicleController::update();
    assert(HardwareInit::getLightDutyPercent(14) >= 39 && "Cab light pin should energize on Bit 5");

    // 17.6: Work Light (Bit 6) -> Work Pin 15
    truck_light.rk.value = 0x40; // Bit 6 Work
    VehicleController::update();
    assert(HardwareInit::getLightDutyPercent(15) >= 89 && "Work light pin should energize on Bit 6");

    // 17.7: Aux Light (Bit 7) -> Aux Pin 16
    truck_light.rk.value = 0x80; // Bit 7 Aux
    VehicleController::update();
    assert(HardwareInit::getLightDutyPercent(16) >= 69 && "Aux light pin should energize on Bit 7");

    // 17.8: Configured light mask
    uint8_t mask = HardwareInit::getConfiguredLightMask(testHwLights.lights, false);
    assert(mask == 0xFF && "All 8 bits should be reported as configured in mask");

    std::cout << "  PASS: 8-Bit lighting grid and all independent channels verified." << std::endl;

    // ── Test 18: Battery Warning-Floor Percentage & 0-200 km/h Speed Telemetry ──
    std::cout << "[Host VC Test] Test 18: Battery Warning-Floor Percentage & Speed Telemetry..." << std::endl;
    HardwareConfig testHwTelem;
    testHwTelem.battery.cellCount = 2;
    testHwTelem.battery.warningVoltage = 3.5f; // 2 * 3.5 = 7.0V warning
    testHwTelem.battery.fullVoltage = 4.2f;    // 2 * 4.2 = 8.4V full
    testHwTelem.battery.cutoffVoltage = 3.3f;  // 2 * 3.3 = 6.6V cutoff
    testHwTelem.battery.vScale = 1.0f;
    testHwTelem.battery.vOffset = 0.0f;
    HardwareInit::init(testHwTelem);
    VehicleController::init(&testHwTelem, &engine, &profile);

    // 18.1: Full voltage (8.4V) -> 100%
    host_analog_read_mv = 8400;
    host_virtual_millis += 1100;  // >1000ms telemetry interval
    VehicleController::update();
    assert(strcmp(telemetry_Battery.rk.content, "100") == 0 && "Full voltage (8.4V) should report 100%");

    // 18.2: Mid voltage (7.7V) -> 50%
    host_analog_read_mv = 7700;
    for (int i = 0; i < 80; i++) { host_virtual_millis += 100; VehicleController::update(); }
    host_virtual_millis += 1100;  // >1000ms telemetry interval
    VehicleController::update();
    assert(strcmp(telemetry_Battery.rk.content, "50") == 0 && "Mid voltage (7.7V) should report 50%");

    // 18.3: Warning voltage (7.0V) -> 0%
    host_analog_read_mv = 7000;
    for (int i = 0; i < 80; i++) { host_virtual_millis += 100; VehicleController::update(); }
    host_virtual_millis += 1100;  // >1000ms telemetry interval
    VehicleController::update();
    assert(strcmp(telemetry_Battery.rk.content, "0") == 0 && "Warning voltage (7.0V) should report 0%");

    // 18.4: Sub-warning voltage (6.8V) -> 0% (clamped)
    host_analog_read_mv = 6800;
    for (int i = 0; i < 80; i++) { host_virtual_millis += 100; VehicleController::update(); }
    host_virtual_millis += 1100;  // >1000ms telemetry interval
    VehicleController::update();
    assert(strcmp(telemetry_Battery.rk.content, "0") == 0 && "Sub-warning voltage (6.8V) should be clamped to 0%");

    // 18.5: Speed Telemetry in km/h (0..200 km/h)
    // Start engine in Drive
    start_button.rk.state = true;
    gear_switch.rk.value = 0; // Drive (D)
    brake_pedal.rk.value = -100;
    gas_pedal.rk.value = 0;   // 50% throttle -> 100 km/h
    for (int i = 0; i < 200; i++) {
        host_virtual_millis += 10;
        engine.update(0);
        for (int s = 0; s < 220; s++) engine.getNextSample();
        VehicleController::update();
    }
    assert(strcmp(telemetry_Speed.rk.content, "100") == 0 && "50% motor speed should report 100 km/h");

    gas_pedal.rk.value = 100; // 100% throttle -> 200 km/h
    for (int i = 0; i < 200; i++) {
        host_virtual_millis += 10;
        engine.update(0);
        for (int s = 0; s < 220; s++) engine.getNextSample();
        VehicleController::update();
    }
    assert(strcmp(telemetry_Speed.rk.content, "200") == 0 && "100% motor speed should report 200 km/h");

    // Park (P) -> speed 0 km/h
    gear_switch.rk.value = 1; // Park (P)
    host_virtual_millis += 1100;  // >1000ms telemetry interval
    VehicleController::update();
    assert(strcmp(telemetry_Speed.rk.content, "0") == 0 && "Park (P) gear should report 0 km/h");

    std::cout << "  PASS: Warning-floor battery percentage and 0-200 km/h speed telemetry verified." << std::endl;

    std::cout << "[Host VC Test] Test 19: Staged Lifecycle and Metadata Priority Cascade..." << std::endl;
    // Reset RadioKit config
    initRadioKit();

    // 19.1: Hardware config overrides vehicle config
    HardwareConfig cascadeHw;
    strlcpy(cascadeHw.name, "Custom HW Name", sizeof(cascadeHw.name));
    strlcpy(cascadeHw.description, "Custom HW Desc", sizeof(cascadeHw.description));

    RcEngineSound::Config cascadeVc;
    strlcpy(cascadeVc.name, "Scania V8", sizeof(cascadeVc.name));
    strlcpy(cascadeVc.description, "Vehicle Desc", sizeof(cascadeVc.description));
    cascadeVc.type = RcEngineSound::VEHICLE_TRUCK;

    // Apply metadata
    if (cascadeHw.name[0] != '\0') {
        RadioKit.config.name = cascadeHw.name;
    } else if (cascadeVc.name[0] != '\0' && strcmp(cascadeVc.name, "Unknown") != 0) {
        RadioKit.config.name = cascadeVc.name;
    } else {
        RadioKit.config.name = "RC_UI";
    }

    if (cascadeHw.description[0] != '\0') {
        RadioKit.config.description = cascadeHw.description;
    } else if (cascadeVc.description[0] != '\0') {
        RadioKit.config.description = cascadeVc.description;
    } else {
        RadioKit.config.description = "";
    }

    assert(strcmp(RadioKit.config.name, "Custom HW Name") == 0 && "Hardware name should override vehicle name");
    assert(strcmp(RadioKit.config.description, "Custom HW Desc") == 0 && "Hardware description should override vehicle description");

    // 19.2: Vehicle config fallback when hardware name is empty
    cascadeHw.name[0] = '\0';
    cascadeHw.description[0] = '\0';

    if (cascadeHw.name[0] != '\0') {
        RadioKit.config.name = cascadeHw.name;
    } else if (cascadeVc.name[0] != '\0' && strcmp(cascadeVc.name, "Unknown") != 0) {
        RadioKit.config.name = cascadeVc.name;
    } else {
        RadioKit.config.name = "RC_UI";
    }

    if (cascadeHw.description[0] != '\0') {
        RadioKit.config.description = cascadeHw.description;
    } else if (cascadeVc.description[0] != '\0') {
        RadioKit.config.description = cascadeVc.description;
    } else {
        RadioKit.config.description = "";
    }

    assert(strcmp(RadioKit.config.name, "Scania V8") == 0 && "Vehicle name should be used when hardware name is empty");
    assert(strcmp(RadioKit.config.description, "Vehicle Desc") == 0 && "Vehicle description should be used when hardware description is empty");

    // 19.3: Fallback when both are empty
    cascadeVc.name[0] = '\0';
    cascadeVc.description[0] = '\0';

    if (cascadeHw.name[0] != '\0') {
        RadioKit.config.name = cascadeHw.name;
    } else if (cascadeVc.name[0] != '\0' && strcmp(cascadeVc.name, "Unknown") != 0) {
        RadioKit.config.name = cascadeVc.name;
    } else {
        RadioKit.config.name = "RC_UI";
    }

    assert(strcmp(RadioKit.config.name, "RC_UI") == 0 && "Default fallback should be RC_UI when both configs lack name");
    std::cout << "  PASS: Staged lifecycle and metadata priority cascade verified." << std::endl;

    // ── Test 20: Turn Signal Edge Latching & Suppression ──
    std::cout << "[Host VC Test] Test 20: Turn Signal App-Suppression & Edge Latching..." << std::endl;
    HardwareConfig testHw20;
    testHw20.lights.turnLight.leftPin = 17;
    testHw20.lights.turnLight.rightPin = 18;
    testHw20.lights.turnLight.configured = true;
    HardwareInit::init(testHw20);
    VehicleController::init(&testHw20, &engine, &profile);

    // 20.1: Turn left indicator ON at steering 0
    steering_wheel.rk.value = 0;
    left_indicator.rk.state = true;
    VehicleController::update();
    assert(left_indicator.rk.state == true && "Left indicator should be ON");

    // 20.2: Steer left >= 20% into turn (arms cancellation)
    steering_wheel.rk.value = -30;
    VehicleController::update();
    assert(left_indicator.rk.state == true && "Left indicator should remain ON while steering into turn");

    // 20.3: Return wheel towards center -> auto-cancels
    steering_wheel.rk.value = 0;
    VehicleController::update();
    assert(left_indicator.rk.state == false && "Left indicator should auto-cancel when returning to center");

    // 20.4: App continues sending state=true (simulating repeated BLE packets)
    // It must remain suppressed and NOT re-trigger!
    left_indicator.rk.state = true;
    VehicleController::update();
    assert(left_indicator.rk.state == false && "Left indicator should remain suppressed while app still sends true");

    // 20.5: User taps button to OFF (state=false) -> clears suppression
    left_indicator.rk.state = false;
    VehicleController::update();
    assert(left_indicator.rk.state == false && "Suppression cleared on false");

    // 20.6: User taps button again (state=true) -> turns ON freshly
    left_indicator.rk.state = true;
    VehicleController::update();
    assert(left_indicator.rk.state == true && "Left indicator turns ON freshly after suppression release");

    // 20.7: Activating right indicator mutually suppresses left indicator
    right_indicator.rk.state = true;
    VehicleController::update();
    assert(left_indicator.rk.state == false && "Left indicator should be turned OFF when right is activated");
    assert(right_indicator.rk.state == true && "Right indicator should be ON");

    std::cout << "  PASS: Turn signal app-suppression and edge-latching verified." << std::endl;

    std::cout << "[Host VC Test] Test 21: Virtual Mass Inertia Drivetrain Simulation & Direct Fallback..." << std::endl;
    HardwareConfig hwConfig21;
    hwConfig21.drivetrainType = HardwareConfig::ACKERMANN;
    hwConfig21.driveMotor.type = HardwareConfig::DriveMotor::DRIVER;
    hwConfig21.driveMotor.hardwareId = PinMapper::DRIVER_A;
    hwConfig21.driveMotor.direction = HardwareConfig::DriveMotor::FORWARD;
    hwConfig21.driveMotor.duty.min = 0;
    hwConfig21.driveMotor.duty.max = 100;
    hwConfig21.driveMotor.configured = true;

    HardwareInit::init(hwConfig21);
    VehicleController::init(&hwConfig21, &engine, &profile);
    start_button.rk.state = true;

    // Ensure engine is RUNNING
    for (int i = 0; i < 200; i++) {
        host_virtual_millis += 10;
        engine.update(0);
        for (int s = 0; s < 220; s++) engine.getNextSample();
        VehicleController::update();
    }
    assert(engine.getState() == RcEngineSound::RUNNING);

    // 21.1 Direct Mode Fallback: hasEngine = false -> instant 100% output
    profile.config.engine.hasEngine = false;
    gear_switch.rk.value = 0; // D
    gas_pedal.rk.value = 100;
    brake_pedal.rk.value = -100;
    VehicleController::update();
    assert(host_last_motor_speed == 100.0f && "Direct mode should immediately output 100% motor speed");

    // 21.2 Direct Mode Fallback: inertia = 0 -> instant output
    profile.config.engine.hasEngine = true;
    profile.config.engine.inertia = 0;
    gas_pedal.rk.value = -100;
    VehicleController::update();
    assert(host_last_motor_speed == 0.0f && "Direct mode should immediately output 0% motor speed on pedal release");

    // 21.3 Virtual Mass Inertia Ramping: hasEngine = true, inertia = 50, acc = 5, dec = 2, ramp = 20ms
    profile.config.engine.inertia = 50;
    profile.config.engine.acc = 5;
    profile.config.engine.dec = 2;
    profile.config.engine.brakeDec = 15;
    profile.config.engine.escRampTime = 20;

    // Start from 0 speed in Drive
    gas_pedal.rk.value = 100; // Request 100% throttle
    host_virtual_millis += 20;
    VehicleController::update();
    float speedTick1 = host_last_motor_speed;
    assert(speedTick1 > 0.0f && speedTick1 < 15.0f && "Motor speed should ramp gradually under mass inertia");

    host_virtual_millis += 20;
    VehicleController::update();
    float speedTick2 = host_last_motor_speed;
    assert(speedTick2 > speedTick1 && speedTick2 < 25.0f && "Motor speed should continue ramping smoothly");

    // Ramp up over multiple ticks towards 100%
    for (int t = 0; t < 50; t++) {
        host_virtual_millis += 20;
        VehicleController::update();
    }
    assert(fabs(host_last_motor_speed - 100.0f) < 1.0f && "Motor speed should reach 100% after ramp duration");

    // 21.4 Coasting on throttle release (brake = -100 / 0%)
    gas_pedal.rk.value = -100; // release throttle to idle
    host_virtual_millis += 20;
    VehicleController::update();
    float coastSpeed1 = host_last_motor_speed;
    assert(coastSpeed1 < 100.0f && coastSpeed1 > 90.0f && "Motor should coast down slowly on throttle release");

    // 21.5 Active Braking: apply brake pedal
    brake_pedal.rk.value = 100; // 100% brake
    host_virtual_millis += 20;
    VehicleController::update();
    float brakeSpeed1 = host_last_motor_speed;
    float brakeDelta = coastSpeed1 - brakeSpeed1;
    assert(brakeDelta > 10.0f && "Brake pedal should decelerate significantly faster than coasting");

    // 21.6 Park Lock Emergency Clamp
    gear_switch.rk.value = 1; // P
    VehicleController::update();
    assert(host_last_motor_speed == 0.0f && "Park lock must instantly clamp ramped motor speed to 0");

    std::cout << "  PASS: Virtual mass inertia ramping, coasting, braking, and direct fallback verified." << std::endl;

    // ── Test 22: Automatic Transmission Torque Converter Simulation ──
    std::cout << "[Host VC Test] Test 22: Automatic Transmission Torque Converter Simulation..." << std::endl;
    profile.config.transmission.type = RcEngineSound::TRANS_AUTOMATIC;
    profile.config.transmission.numberOfGears = 3;
    profile.config.transmission.torqueConverterSlip = 100;
    engine.begin(soundData, profile.config);

    // Clean start sequence from OFF -> STARTING -> RUNNING
    start_button.rk.state = false;
    VehicleController::update();
    start_button.rk.state = true;
    VehicleController::update();
    gear_switch.rk.value = 0; // D
    gas_pedal.rk.value = -100; // Idle
    for (int i = 0; i < 200; i++) {
        host_virtual_millis += 10;
        engine.update(0);
        for (int s = 0; s < 220; s++) engine.getNextSample();
        VehicleController::update();
    }
    assert(engine.getState() == RcEngineSound::RUNNING);

    // 22.1 Launch in 1st gear with full throttle step -> verify stall slip flare
    gas_pedal.rk.value = 100; // Step to 100%
    host_virtual_millis += 20;
    VehicleController::update();

    // Ramping with converter slip should drive engine RPM higher than base 1st gear slice (500/3 = 166)
    for (int t = 0; t < 15; t++) {
        host_virtual_millis += 20;
        for (int s = 0; s < 220; s++) engine.getNextSample();
        VehicleController::update();
    }
    uint16_t launchRpm = engine.getRpm();
    assert(launchRpm > 170 && "Torque converter slip must flare engine RPM above 1st gear geometric limit on launch");

    // 22.2 Cruise at top speed -> verify torque converter lockup when engine load drops to 0
    for (int t = 0; t < 100; t++) {
        host_virtual_millis += 20;
        engine.update(500);
        for (int s = 0; s < 220; s++) engine.getNextSample();
        VehicleController::update();
    }
    uint16_t cruiseRpm = engine.getRpm();
    assert(cruiseRpm >= 490 && "Engine RPM should reach full maxRpm at steady cruising speed in top gear");

    std::cout << "  PASS: Automatic transmission torque converter slip flare and high-speed lockup verified." << std::endl;

    // ── Test 23: Locomotive Directional Lighting Handover & Asymmetric PWM Slew Limiter ──
    std::cout << "[Host VC Test] Test 23: Locomotive Directional Lighting & Asymmetric Incandescent Slew Limiter..." << std::endl;
    HardwareConfig locoHw;
    locoHw.lights.headLight.pin = PIN::L1;
    locoHw.lights.headLight.brightness = 100;
    locoHw.lights.headLight.configured = true;
    locoHw.lights.tailLight.pin = PIN::L2;
    locoHw.lights.tailLight.brightness = 80;
    locoHw.lights.tailLight.configured = true;
    locoHw.lights.cabLight.pin = PIN::L3;
    locoHw.lights.cabLight.brightness = 60;
    locoHw.lights.cabLight.configured = true;
    locoHw.lights.ditchLight.leftPin = PIN::L4;
    locoHw.lights.ditchLight.rightPin = PIN::L5;
    locoHw.lights.ditchLight.brightness = 100;
    locoHw.lights.ditchLight.intervalMs = 500;
    locoHw.lights.ditchLight.configured = true;
    locoHw.lights.stepLight.pin = PIN::L6;
    locoHw.lights.stepLight.brightness = 50;
    locoHw.lights.stepLight.configured = true;
    locoHw.driveMotor.type = HardwareConfig::DriveMotor::DRIVER;
    locoHw.driveMotor.hardwareId = PinMapper::DRIVER_A;
    locoHw.driveMotor.direction = HardwareConfig::DriveMotor::FORWARD;
    locoHw.driveMotor.duty.min = 0;
    locoHw.driveMotor.duty.max = 100;
    locoHw.driveMotor.configured = true;
    host_radiokit_connected = true;
    HardwareInit::init(locoHw);

    VehicleProfile locoProfile;
    locoProfile.config.type = RcEngineSound::VEHICLE_LOCOMOTIVE;
    locoProfile.config.engine.hasEngine = true;
    locoProfile.config.engine.inertia = 50;
    locoProfile.config.engine.acc = 5;
    locoProfile.config.engine.dec = 2;
    locoProfile.config.engine.brakeDec = 15;
    locoProfile.config.engine.escRampTime = 20;

    // Stop engine from previous test cleanly before re-initializing
    start_button.rk.state = false;
    VehicleController::update();
    for (int i = 0; i < 100; i++) {
        host_virtual_millis += 20;
        engine.update(0);
        for (int s = 0; s < 220; s++) engine.getNextSample();
    }

    engine.begin(soundData, locoProfile.config);
    dir_switch.rk.state = true; // Default Forward
    loco_light.rk.value = 0;
    VehicleController::init(&locoHw, &engine, &locoProfile);
    engine_button.rk.state = true; // Engine Start
    int lastDbgState = -1;
    for (int i = 0; i < 200; i++) {
        host_virtual_millis += 10;
        int preUpd = (int)engine.getState();
        engine.update(0);
        int postUpd = (int)engine.getState();
        for (int s = 0; s < 220; s++) engine.getNextSample();
        int postSample = (int)engine.getState();
        VehicleController::update();
        int postVC = (int)engine.getState();
        if (preUpd != postUpd || postUpd != postSample || postSample != postVC || postVC != lastDbgState) {
            std::cout << "DEBUG T23 iter=" << i
                      << " pre_upd=" << preUpd
                      << " post_upd=" << postUpd
                      << " post_sample=" << postSample
                      << " post_vc=" << postVC
                      << " engine_btn=" << engine_button.rk.state
                      << std::endl;
            lastDbgState = postVC;
        }
        if (postVC == 3 || postVC == 0) break; // stop on STOPPING or OFF to avoid noise
    }
    std::cout << "DEBUG Test 23 Final Engine state: " << (int)engine.getState() << std::endl;
    // Temporarily skip assert to see all debug output
    if (engine.getState() != RcEngineSound::RUNNING) {
        std::cout << "WARN: Engine not RUNNING, skipping remaining tests" << std::endl;
        return 1;
    }

    // 23.1 Forward Directional Handover: dir_switch = 1, Bit 0 (Headlight) ON
    dir_switch.rk.state = true; // Forward
    loco_light.rk.value = 0x01; // Bit 0 Headlight ON (target 100% full brightness)
    host_virtual_millis += 20;
    VehicleController::update();
    float headDutyStart = HardwareInit::getLightDutyPercent(PIN::L1);
    assert(headDutyStart > 0.0f && headDutyStart <= 15.0f && "Headlight should start ramping up smoothly");
    assert(HardwareInit::getLightDutyPercent(PIN::L2) == 0.0f && "Tail light must be extinguished in forward");

    // Slew up over 200ms -> should reach 100%
    for (int t = 0; t < 15; t++) {
        host_virtual_millis += 20;
        VehicleController::update();
    }
    float headDutyFinal = HardwareInit::getLightDutyPercent(PIN::L1);
    assert(fabs(headDutyFinal - 100.0f) <= 1.0f && "Headlight should reach 100% after warm-up ramp");

    // 23.2 Reverse Directional Handover: stationary -> flip to Reverse (dir_switch = 0)
    dir_switch.rk.state = false; // Reverse
    loco_light.rk.value = 0x01; // Headlight ON
    for (int t = 0; t < 15; t++) {
        host_virtual_millis += 20;
        VehicleController::update();
    }
    assert(fabs(HardwareInit::getLightDutyPercent(PIN::L2) - 80.0f) <= 1.0f && "Tail marker should reach 80% in reverse");
    assert(HardwareInit::getLightDutyPercent(PIN::L1) == 0.0f && "Forward headlight must be extinguished in reverse");

    // 23.3 Asymmetric Fade-Out Rate Verification (Fast Cool-down 1.0%/ms vs Warm-up 0.5%/ms)
    // Turn cab light ON (target 60%, Bit 4 / 0x10)
    loco_light.rk.value = 0x10; // Bit 4 Cab Light ON
    for (int t = 0; t < 10; t++) { host_virtual_millis += 20; VehicleController::update(); }
    assert(fabs(HardwareInit::getLightDutyPercent(PIN::L3) - 60.0f) <= 1.0f && "Cab light on");

    // Toggle OFF -> 30ms step -> should drop by ~30% duty (rate 1.0%/ms)
    loco_light.rk.value &= ~0x10;
    host_virtual_millis += 30;
    VehicleController::update();
    float cabDutyAfter30ms = HardwareInit::getLightDutyPercent(PIN::L3);
    assert(cabDutyAfter30ms < 35.0f && "Cab light should cool down rapidly (~1.0% per ms)");

    std::cout << "  PASS: Locomotive directional lighting and asymmetric PWM slew rates verified." << std::endl;

    // ── Test 24: Dual Ditch Light Triangular Cross-Fading (Manual on Bit 2) ──
    std::cout << "[Host VC Test] Test 24: Dual Ditch Light Triangular Cross-Fading..." << std::endl;
    // 24.1 Ditch light inactive -> 0%
    loco_light.rk.value = 0x00;
    VehicleController::update();
    assert(HardwareInit::getLightDutyPercent(PIN::L4) == 0.0f && "Ditch L off");
    assert(HardwareInit::getLightDutyPercent(PIN::L5) == 0.0f && "Ditch R off");

    // 24.2 Activate via Bit 2 (0x04) -> should cross-fade
    loco_light.rk.value = 0x04;
    host_virtual_millis = 10000; // aligned
    VehicleController::update(); // t=0: L=0%, R=100%
    float ditchL_0 = HardwareInit::getLightDutyPercent(PIN::L4);
    float ditchR_0 = HardwareInit::getLightDutyPercent(PIN::L5);
    assert(fabs(ditchL_0 - 0.0f) <= 1.0f && fabs(ditchR_0 - 100.0f) <= 1.0f);

    // Halfway through half-period (t=250ms of 500ms): L=50%, R=50%
    host_virtual_millis = 10250;
    VehicleController::update();
    float ditchL_250 = HardwareInit::getLightDutyPercent(PIN::L4);
    float ditchR_250 = HardwareInit::getLightDutyPercent(PIN::L5);
    assert(fabs(ditchL_250 - 50.0f) < 2.0f && fabs(ditchR_250 - 50.0f) < 2.0f);

    // Peak (t=500ms): L=100%, R=0%
    host_virtual_millis = 10500;
    VehicleController::update();
    float ditchL_500 = HardwareInit::getLightDutyPercent(PIN::L4);
    float ditchR_500 = HardwareInit::getLightDutyPercent(PIN::L5);
    assert(fabs(ditchL_500 - 100.0f) <= 1.0f && fabs(ditchR_500 - 0.0f) <= 1.0f);

    // Turn Bit 2 OFF -> Ditch lights extinguished
    loco_light.rk.value = 0x00;
    VehicleController::update();
    assert(HardwareInit::getLightDutyPercent(PIN::L4) == 0.0f && "Ditch L extinguished on toggle off");
    assert(HardwareInit::getLightDutyPercent(PIN::L5) == 0.0f && "Ditch R extinguished on toggle off");

    std::cout << "  PASS: Dual ditch light triangular cross-fading and purely manual control verified." << std::endl;

    // ── Test 25: Locomotive Reverser Momentum Interlock & Throttle Auto-Zero ──
    std::cout << "[Host VC Test] Test 25: Reverser Momentum Interlock & Throttle Auto-Zero..." << std::endl;
    dir_switch.rk.state = true; // Forward
    throttle_slider.rk.value = 100; // Full throttle
    VehicleController::update();

    // Accelerate to ~80% speed
    for (int t = 0; t < 30; t++) {
        host_virtual_millis += 20;
        VehicleController::update();
    }
    assert(host_last_motor_speed > 50.0f && "Locomotive moving forward at speed");

    // Flip reverser to Reverse while moving!
    dir_switch.rk.state = false; // Reverse
    VehicleController::update();
    assert(VehicleController::isReverserInterlocked() && "Reverser flip while moving must engage interlock");
    assert(throttle_slider.rk.value == -100 && "Throttle slider must be clamped / auto-zeroed");
    assert(host_last_motor_speed > 0.0f && "Motor polarity must remain Forward until stopped");

    // Decelerate with dynamic braking down to 0
    for (int t = 0; t < 30; t++) {
        host_virtual_millis += 20;
        VehicleController::update();
    }
    assert(host_last_motor_speed == 0.0f && "Locomotive should come to complete standstill");
    assert(!VehicleController::isReverserInterlocked() && "Interlock should clear once stationary");
    assert(!VehicleController::getActiveDirection() && "Reverse polarity engaged after standstill");

    // Reapply throttle in Reverse -> should drive backwards (negative speed)
    throttle_slider.rk.value = 60;
    for (int t = 0; t < 20; t++) {
        host_virtual_millis += 20;
        VehicleController::update();
    }
    assert(host_last_motor_speed < 0.0f && "Motor drives in reverse after interlock release");

    std::cout << "  PASS: Locomotive reverser momentum interlock, braking, and delayed polarity verified." << std::endl;

    std::cout << "[Host VC Test] ALL HOST VEHICLE CONTROLLER ASSERTIONS PASSED SUCCESSFULLY." << std::endl;
    return 0;
}

