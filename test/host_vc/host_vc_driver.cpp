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
extern float host_motor_writes[8];
extern size_t host_motor_write_count;
extern int host_last_servo_us;

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
    left_indicator.rk.state = true;
    right_indicator.rk.state = false;
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
    host_virtual_millis += 300;
    VehicleController::update();
    assert(strcmp(telemetry_Battery.rk.content, "100") == 0 && "Full voltage (8.4V) should report 100%");

    // 18.2: Mid voltage (7.7V) -> 50%
    host_analog_read_mv = 7700;
    host_virtual_millis += 300;
    VehicleController::update();
    assert(strcmp(telemetry_Battery.rk.content, "50") == 0 && "Mid voltage (7.7V) should report 50%");

    // 18.3: Warning voltage (7.0V) -> 0%
    host_analog_read_mv = 7000;
    host_virtual_millis += 300;
    VehicleController::update();
    assert(strcmp(telemetry_Battery.rk.content, "0") == 0 && "Warning voltage (7.0V) should report 0%");

    // 18.4: Sub-warning voltage (6.8V) -> 0% (clamped)
    host_analog_read_mv = 6800;
    host_virtual_millis += 300;
    VehicleController::update();
    assert(strcmp(telemetry_Battery.rk.content, "0") == 0 && "Sub-warning voltage (6.8V) should be clamped to 0%");

    // 18.5: Speed Telemetry in km/h (0..200 km/h)
    // Start engine in Drive
    start_button.rk.state = true;
    gear_switch.rk.value = 0; // Drive (D)
    gas_pedal.rk.value = 50;  // 50% throttle -> 100 km/h
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
    host_virtual_millis += 300;
    VehicleController::update();
    assert(strcmp(telemetry_Speed.rk.content, "0") == 0 && "Park (P) gear should report 0 km/h");

    std::cout << "  PASS: Warning-floor battery percentage and 0-200 km/h speed telemetry verified." << std::endl;

    std::cout << "[Host VC Test] ALL HOST VEHICLE CONTROLLER ASSERTIONS PASSED SUCCESSFULLY." << std::endl;
    return 0;
}

