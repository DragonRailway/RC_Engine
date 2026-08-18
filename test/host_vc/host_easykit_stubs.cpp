#include "Arduino.h"
#include <LittleFS.h>
#include "boards.h"
#include "Config.h"
#include <EasyMotor.h>
#include <EasyServo.h>
#include <EasyLED.h>
#include <RadioKitLib.h>
#include <iostream>

// Captured state variables for assertions
float host_last_motor_speed = 0.0f;
// Ring of every motor write (drive/left + aux/right channels), for skid-steer
// differential assertions: writes[0]=left, writes[1]=right within one update.
float host_motor_writes[8] = {0.0f};
size_t host_motor_write_count = 0;
int host_last_servo_us = 0;
int host_last_aux1_us = 0;
int host_last_aux2_us = 0;

uint8_t host_last_head_led = 0;
uint8_t host_last_tail_led = 0;
uint8_t host_last_brake_led = 0;

uint8_t host_gpio_pin_mode[128] = {0};
uint8_t host_gpio_pin_val[128] = {0};
uint32_t host_analog_read_mv = 3960;

// Dummy EasyMotor
EasyMotor::EasyMotor() {}
EasyMotor::EasyMotor(uint8_t, uint8_t) {}
EasyMotor::EasyMotor(uint8_t, uint8_t, uint8_t) {}
EasyMotor::EasyMotor(DriverType, uint8_t, uint8_t) {}
EasyMotor::EasyMotor(DriverType, uint8_t, uint8_t, uint8_t) {}
EasyMotor::~EasyMotor() {}

EasyKit::Result EasyMotor::begin(uint8_t, uint8_t, uint8_t, bool) { return EasyKit::Result::OK; }
EasyKit::Result EasyMotor::begin(uint8_t, uint8_t, bool) { return EasyKit::Result::OK; }
EasyKit::Result EasyMotor::begin(DriverType, uint8_t, uint8_t, uint8_t, bool) { return EasyKit::Result::OK; }
EasyKit::Result EasyMotor::begin(DriverType, uint8_t, uint8_t, bool) { return EasyKit::Result::OK; }
EasyKit::Result EasyMotor::begin(const EasyKit::EasyMotorConfig&) { return EasyKit::Result::OK; }
void EasyMotor::end() {}
void EasyMotor::setFrequency(uint32_t) {}
void EasyMotor::write(float speed) {
    host_last_motor_speed = speed;
    if (host_motor_write_count < 8) host_motor_writes[host_motor_write_count++] = speed;
}
void EasyMotor::stop() { host_last_motor_speed = 0.0f; }
void EasyMotor::brake() { host_last_motor_speed = 0.0f; }
void EasyMotor::coast() { host_last_motor_speed = 0.0f; }
void EasyMotor::onPinStolen(uint8_t) {}

// Dummy EasyServo
EasyServo::EasyServo() {}
EasyServo::~EasyServo() {}
int EasyServo::attach(int pin) { return 0; }
int EasyServo::attach(int pin, int minUs, int maxUs) { return 0; }
EasyKit::Result EasyServo::attach(int pin, const EasyKit::ServoConfig& config) { return EasyKit::Result::OK; }
void EasyServo::detach() {}
float EasyServo::write(float value, float speed, float kIn, float kOut) { return value; }
void EasyServo::writeMicroseconds(int us) { host_last_servo_us = us; }
void EasyServo::update() {}
void EasyServo::stop() {}
int EasyServo::read() const { return 90; }
int EasyServo::readMicroseconds() const { return 1500; }
bool EasyServo::attached() const { return true; }
void EasyServo::onPinStolen(uint8_t) {}

// Dummy EasyLED
struct EasyLED_StubState {
    float dutyPct = 0.0f;
};
EasyLED::EasyLED() {}
EasyLED::EasyLED(uint8_t pin) {}
EasyLED::~EasyLED() {}
EasyKit::Result EasyLED::begin(uint8_t pin, const EasyKit::LEDConfig& config) { return EasyKit::Result::OK; }
void EasyLED::end() {}
void EasyLED::write(bool value) { write(value ? 100.0f : 0.0f); }
void EasyLED::write(uint16_t ticks) { write(static_cast<float>(ticks) * 100.0f / 1023.0f); }
void EasyLED::write(float percent) {
    host_last_head_led = (uint8_t)percent;
    _duty = (uint32_t)(percent * 10.23f + 0.5f);
}

void EasyLED::setDuty(uint32_t duty) { _duty = duty; }
void EasyLED::update() {}
void EasyLED::stop() { write(0.0f); }
void EasyLED::startBlink(uint32_t onMs, uint32_t offMs, float duty) { write(duty); }
void EasyLED::stopBlink() { write(0.0f); }
bool EasyLED::fadeTo(uint32_t target, uint32_t, EasyLED::Curve, void (*)(void*), void*) { _duty = target; return true; }
float EasyLED::getDutyPercent() const { return (float)_duty * 100.0f / 1023.0f; }

uint32_t EasyLED::getMaxDuty() const { return 1023; }

bool EasyLED::isAttached() const { return true; }
void EasyLED::onPinStolen(uint8_t) {}

// RadioKit Global Stubs
bool host_radiokit_connected = false;
RadioKitClass::RadioKitClass() {}
RadioKitClass RadioKit;
void RadioKitClass::_registerWidget(RadioKit_Widget*) {}
void RadioKitClass::pushUpdate(uint8_t) {}
void RadioKitClass::markConfDirty() {}
void RadioKitClass::startSerial(Stream&) {}
void RadioKitClass::begin() {}
void RadioKitClass::startBLE(const char*) {}
bool RadioKitClass::enableFS() { return true; }
void RadioKitClass::update() {}
bool RadioKitClass::isConnected() const { return host_radiokit_connected; }
