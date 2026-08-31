#include "HardwareInit.h"
#ifdef ESP32
#include <esp_sleep.h>
#endif

// ─── Static member definitions ────────────────────────────────────────────────

bool     HardwareInit::s_powerLatched = false;
bool     HardwareInit::s_powerButtonHolding = false;
uint32_t HardwareInit::s_powerButtonHoldStart = 0;
bool     HardwareInit::s_buttonClicked = false;

HardwareInit::MotorChannel HardwareInit::s_driveCh[HardwareConfig::MAX_DRIVE_MOTORS];
uint8_t HardwareInit::s_driveChCount = 0;
HardwareInit::MotorChannel HardwareInit::s_rightCh;
uint8_t HardwareInit::s_primaryBemfPin = 0xFF;

uint8_t HardwareInit::headPins[HardwareConfig::MAX_PINS_PER_LIGHT];
uint8_t HardwareInit::headPinCount = 0;
uint8_t HardwareInit::fullPins[HardwareConfig::MAX_PINS_PER_LIGHT];
uint8_t HardwareInit::fullPinCount = 0;
uint8_t HardwareInit::fogPins[HardwareConfig::MAX_PINS_PER_LIGHT];
uint8_t HardwareInit::fogPinCount = 0;
uint8_t HardwareInit::tailPins[HardwareConfig::MAX_PINS_PER_LIGHT];
uint8_t HardwareInit::tailPinCount = 0;
uint8_t HardwareInit::brakePins[HardwareConfig::MAX_PINS_PER_LIGHT];
uint8_t HardwareInit::brakePinCount = 0;
uint8_t HardwareInit::reversingPins[HardwareConfig::MAX_PINS_PER_LIGHT];
uint8_t HardwareInit::reversingPinCount = 0;
uint8_t HardwareInit::beaconPins[HardwareConfig::MAX_PINS_PER_LIGHT];
uint8_t HardwareInit::beaconPinCount = 0;
uint8_t HardwareInit::cabPins[HardwareConfig::MAX_PINS_PER_LIGHT];
uint8_t HardwareInit::cabPinCount = 0;
uint8_t HardwareInit::workPins[HardwareConfig::MAX_PINS_PER_LIGHT];
uint8_t HardwareInit::workPinCount = 0;
uint8_t HardwareInit::stepPins[HardwareConfig::MAX_PINS_PER_LIGHT];
uint8_t HardwareInit::stepPinCount = 0;
uint8_t HardwareInit::auxPins[HardwareConfig::MAX_PINS_PER_LIGHT];
uint8_t HardwareInit::auxPinCount = 0;

uint8_t HardwareInit::turnLPin = 0xFF;
uint8_t HardwareInit::turnRPin = 0xFF;
uint8_t HardwareInit::ditchLPin = 0xFF;
uint8_t HardwareInit::ditchRPin = 0xFF;

HardwareConfig::AuxMotor HardwareInit::s_auxConfigs[HardwareConfig::MAX_AUX_MOTORS];
uint8_t                  HardwareInit::s_auxMotorCount = 0;

HardwareConfig::SteeringServo HardwareInit::s_steeringConfigs[HardwareConfig::MAX_STEERING_SERVOS];
uint8_t                       HardwareInit::s_steeringServoCount = 0;

float    HardwareInit::s_easingSpeedDegS = 180.0f;
float    HardwareInit::s_easingKIn = 0.2f;
float    HardwareInit::s_easingKOut = 0.8f;
uint16_t HardwareInit::s_fadeDurationMs = 250;
uint8_t  HardwareInit::s_ditchBrightness = 100;

uint8_t  HardwareInit::s_blinkPin[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
bool     HardwareInit::s_blinkActive[4] = { false, false, false, false };

EasyMotor HardwareInit::s_motorDrivers[HardwareInit::MAX_PHYSICAL_DRIVERS];
EasyServo HardwareInit::steeringServos[HardwareConfig::MAX_STEERING_SERVOS];
EasyServo HardwareInit::escServo;
EasyServo HardwareInit::auxServos[HardwareConfig::MAX_AUX_MOTORS];

EasyMotor* HardwareInit::getDriverForId(uint8_t hardwareId) {
    if (hardwareId >= PinMapper::DRIVER_A && hardwareId <= PinMapper::DRIVER_D) {
        return &s_motorDrivers[hardwareId - PinMapper::DRIVER_A];
    }
    return nullptr;
}

const char* HardwareInit::getDriverName(uint8_t hardwareId) {
    switch (hardwareId) {
        case PinMapper::DRIVER_A: return "DRIVER_A";
        case PinMapper::DRIVER_B: return "DRIVER_B";
        case PinMapper::DRIVER_C: return "DRIVER_C";
        case PinMapper::DRIVER_D: return "DRIVER_D";
        default: return "DRIVER_A";
    }
}

EasyLED   HardwareInit::headLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
EasyLED   HardwareInit::fullLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
EasyLED   HardwareInit::fogLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
EasyLED   HardwareInit::tailLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
EasyLED   HardwareInit::brakeLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
EasyLED   HardwareInit::reversingLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
EasyLED   HardwareInit::beaconLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
EasyLED   HardwareInit::cabLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
EasyLED   HardwareInit::workLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
EasyLED   HardwareInit::stepLeds[HardwareConfig::MAX_PINS_PER_LIGHT];
EasyLED   HardwareInit::auxLeds[HardwareConfig::MAX_PINS_PER_LIGHT];

EasyLED   HardwareInit::turnLLed;
EasyLED   HardwareInit::turnRLed;
EasyLED   HardwareInit::ditchLLed;
EasyLED   HardwareInit::ditchRLed;
EasyLEDGroup HardwareInit::s_ditchGroup;
bool      HardwareInit::s_ditchActive = false;
uint8_t   HardwareInit::s_drivetrainType = HardwareConfig::ACKERMANN;
int16_t   HardwareInit::s_lastServoPos = -999;

// ─── Method implementations ──────────────────────────────────────────────────

void HardwareInit::init(const HardwareConfig& hw) {
    Serial.println("[HardwareInit] Initializing peripherals...");

    s_easingSpeedDegS = hw.animation.easingSpeedDegS;
    s_easingKIn       = hw.animation.easingKIn;
    s_easingKOut      = hw.animation.easingKOut;
    s_fadeDurationMs  = hw.animation.fadeDurationMs;
    resetBlinkTracking();

    for (uint8_t i = 0; i < HardwareConfig::MAX_DRIVE_MOTORS; i++) {
        s_driveCh[i] = MotorChannel();
    }
    s_driveChCount = 0;
    s_rightCh = MotorChannel();
    s_primaryBemfPin = 0xFF;
    s_drivetrainType = hw.drivetrainType;

    if (hw.drivetrainType == HardwareConfig::SKID_STEER) {
        initChannel(s_driveCh[0], hw.leftMotor, getDriverForId(hw.leftMotor.hardwareId), &escServo);
        initChannel(s_rightCh, hw.rightMotor, getDriverForId(hw.rightMotor.hardwareId), &auxServos[0]);
        s_driveChCount = 1;
        s_primaryBemfPin = s_driveCh[0].bemfPin;
    } else {
        s_driveChCount = 0;
        uint8_t effDriveCount = hw.driveMotorCount;
        if (effDriveCount == 0 && hw.driveMotors[0].type != HardwareConfig::DriveMotor::NONE) {
            effDriveCount = 1;
        }
        for (uint8_t i = 0; i < effDriveCount && i < HardwareConfig::MAX_DRIVE_MOTORS; i++) {
            EasyMotor* drv = getDriverForId(hw.driveMotors[i].hardwareId);
            EasyServo* esc = (i == 0) ? &escServo : (i - 1 < HardwareConfig::MAX_AUX_MOTORS ? &auxServos[i - 1] : nullptr);
            initChannel(s_driveCh[i], hw.driveMotors[i], drv, esc);
            if (s_driveCh[i].attached) {
                s_driveChCount++;
            }
        }
        if (s_driveChCount > 0) {
            s_primaryBemfPin = s_driveCh[0].bemfPin;
        }
        initSteeringServos(hw);
    }

    initLights(hw.lights);
    initAuxOutputs(hw);

    Serial.println("[HardwareInit] Done");
}

void HardwareInit::hotReload(const HardwareConfig& hw) {
    Serial.println("[HardwareInit] Hot-reloading...");
    stopAll();
    init(hw);
}

void HardwareInit::stopAll() {
    for (uint8_t i = 0; i < HardwareConfig::MAX_STEERING_SERVOS; i++) {
        steeringServos[i].stop();
        steeringServos[i].detach();
    }
    escServo.stop();
    escServo.detach();
    for (uint8_t i = 0; i < HardwareConfig::MAX_AUX_MOTORS; i++) {
        auxServos[i].stop();
        auxServos[i].detach();
    }
    stopLightAnimations();

    for (uint8_t d = 0; d < MAX_PHYSICAL_DRIVERS; d++) {
        s_motorDrivers[d].end();
    }

    for (uint8_t p = 0; p < headPinCount; p++) headLeds[p].end();
    for (uint8_t p = 0; p < fullPinCount; p++) fullLeds[p].end();
    for (uint8_t p = 0; p < fogPinCount; p++) fogLeds[p].end();
    for (uint8_t p = 0; p < tailPinCount; p++) tailLeds[p].end();
    for (uint8_t p = 0; p < brakePinCount; p++) brakeLeds[p].end();
    for (uint8_t p = 0; p < reversingPinCount; p++) reversingLeds[p].end();
    for (uint8_t p = 0; p < beaconPinCount; p++) beaconLeds[p].end();
    for (uint8_t p = 0; p < cabPinCount; p++) cabLeds[p].end();
    for (uint8_t p = 0; p < workPinCount; p++) workLeds[p].end();
    for (uint8_t p = 0; p < stepPinCount; p++) stepLeds[p].end();
    for (uint8_t p = 0; p < auxPinCount; p++) auxLeds[p].end();
    turnLLed.end();
    turnRLed.end();
    ditchLLed.end();
    ditchRLed.end();

    for (uint8_t i = 0; i < HardwareConfig::MAX_DRIVE_MOTORS; i++) {
        s_driveCh[i].attached = false;
    }
    s_rightCh.attached = false;
    Serial.println("[HardwareInit] Stopped all outputs");
}

void HardwareInit::update(uint16_t buttonHoldS, uint8_t indicatorPin) {
    updatePowerButton(buttonHoldS, indicatorPin);
    for (uint8_t i = 0; i < s_steeringServoCount; i++) {
        steeringServos[i].update();
    }
    escServo.update();
    for (uint8_t i = 0; i < HardwareConfig::MAX_AUX_MOTORS; i++) {
        auxServos[i].update();
    }

    for (uint8_t p = 0; p < headPinCount; p++) headLeds[p].update();
    for (uint8_t p = 0; p < fullPinCount; p++) fullLeds[p].update();
    for (uint8_t p = 0; p < fogPinCount; p++) fogLeds[p].update();
    for (uint8_t p = 0; p < tailPinCount; p++) tailLeds[p].update();
    for (uint8_t p = 0; p < brakePinCount; p++) brakeLeds[p].update();
    for (uint8_t p = 0; p < reversingPinCount; p++) reversingLeds[p].update();
    for (uint8_t p = 0; p < beaconPinCount; p++) beaconLeds[p].update();
    for (uint8_t p = 0; p < cabPinCount; p++) cabLeds[p].update();
    for (uint8_t p = 0; p < workPinCount; p++) workLeds[p].update();
    for (uint8_t p = 0; p < stepPinCount; p++) stepLeds[p].update();
    for (uint8_t p = 0; p < auxPinCount; p++) auxLeds[p].update();

    turnLLed.update();
    turnRLed.update();
    ditchLLed.update();
    ditchRLed.update();
    s_ditchGroup.update();
}

void HardwareInit::latchPower(uint16_t bootLatchS) {
    if (POWER::POWER_ENABLE == 0xFF || POWER::POWER_BUTTON == 0xFF) {
        Serial.println("[HardwareInit] Board lacks power control pins; skipping latchPower()");
        s_powerLatched = true;
        return;
    }
    pinMode(POWER::POWER_ENABLE, OUTPUT);
    digitalWrite(POWER::POWER_ENABLE, LOW);
    pinMode(POWER::POWER_BUTTON, INPUT_PULLDOWN);
    if (POWER::CHARGE_SENS != 0xFF) {
        pinMode(POWER::CHARGE_SENS, INPUT);
    }
    s_powerLatched = false;
    s_powerButtonHolding = false;
    s_powerButtonHoldStart = 0;
    s_buttonClicked = false;

    if (digitalRead(POWER::POWER_BUTTON) == LOW) {
        digitalWrite(POWER::POWER_ENABLE, HIGH);
        s_powerLatched = true;
        Serial.println("[HardwareInit] Power latched via USB boot");
        return;
    }

    uint32_t pressStart = millis();
    uint32_t targetMs = (uint32_t)bootLatchS * 1000U;
    Serial.printf("[HardwareInit] Waiting for power button hold (%ds)...\n", bootLatchS);
    while (digitalRead(POWER::POWER_BUTTON) == HIGH) {
        if (millis() - pressStart >= targetMs) {
            digitalWrite(POWER::POWER_ENABLE, HIGH);
            s_powerLatched = true;
            s_powerButtonHolding = false;
            s_powerButtonHoldStart = 0;
            Serial.println("[HardwareInit] Power successfully latched ON");
            break;
        }
        delay(10);
    }

    if (!s_powerLatched) {
        Serial.println("[HardwareInit] Power button released too early; shutting down");
        digitalWrite(POWER::POWER_ENABLE, LOW);
    }
}

void HardwareInit::updatePowerButton(uint16_t buttonHoldS, uint8_t indicatorPin) {
    if (POWER::POWER_BUTTON == 0xFF) return;

    uint32_t now = millis();
    uint32_t holdMs = (uint32_t)buttonHoldS * 1000U;
    if (digitalRead(POWER::POWER_BUTTON) == HIGH) {
        if (!s_powerButtonHolding) {
            s_powerButtonHolding = true;
            s_powerButtonHoldStart = now;
        }

        // Rapid blink feedback (200ms ON / 200ms OFF) only after confirmed 500ms hold
        if (now - s_powerButtonHoldStart >= 500) {
            uint8_t duty = ((now / 200) % 2 == 0) ? 100 : 0;
            if (indicatorPin != 0xFF) {
                setLight(indicatorPin, duty);
            } else {
                setLight(turnLPin, duty);
                setLight(turnRPin, duty);
            }
        }

        if (now - s_powerButtonHoldStart >= holdMs) {
            Serial.printf("[HardwareInit] %us button hold detected -> powerOff()\n", (unsigned)buttonHoldS);
            s_powerButtonHolding = false;
            s_powerButtonHoldStart = 0;
            if (indicatorPin != 0xFF) {
                setLight(indicatorPin, 0);
            } else {
                setLight(turnLPin, 0);
                setLight(turnRPin, 0);
            }
            powerOff();
        }
    } else {
        if (s_powerButtonHolding) {
            uint32_t duration = now - s_powerButtonHoldStart;
            s_powerButtonHolding = false;
            s_powerButtonHoldStart = 0;

            // Single click: 50ms <= duration < 500ms
            if (duration >= 50 && duration < 500) {
                s_buttonClicked = true;
                Serial.println("[HardwareInit] Power button single-click detected");
            }
        }
    }
}

void HardwareInit::powerOff() {
    Serial.println("[HardwareInit] Powering off hardware...");
    Serial.flush();
    stopAll();
    if (POWER::POWER_ENABLE != 0xFF) {
        digitalWrite(POWER::POWER_ENABLE, LOW);
    }
    s_powerLatched = false;
    s_powerButtonHolding = false;
    s_powerButtonHoldStart = 0;
    s_buttonClicked = false;
    // On battery-powered boards POWER_ENABLE LOW cuts MCU power immediately.
    // On USB-powered boards the MCU stays alive, so we must halt explicitly.
#ifdef ESP32
    esp_deep_sleep(0);  // Sleep forever (wake via GPIO if configured)
#endif
}

void HardwareInit::setSkidMotors(int16_t leftSpeed, int16_t rightSpeed) {
    setChannel(s_driveCh[0], leftSpeed);
    setChannel(s_rightCh, rightSpeed);
}

void HardwareInit::setAllMotors(int16_t speed) {
    for (uint8_t i = 0; i < s_driveChCount; i++) {
        setChannel(s_driveCh[i], speed);
    }
    setChannel(s_rightCh, speed);
}

void HardwareInit::setMotor(int16_t speed) {
    for (uint8_t i = 0; i < s_driveChCount; i++) {
        setChannel(s_driveCh[i], speed);
    }
}

void HardwareInit::setChannel(MotorChannel& ch, int16_t speed) {
    if (ch.type == HardwareConfig::DriveMotor::NONE || !ch.attached) return;

    int16_t eff = speed;
    switch (ch.direction) {
        case HardwareConfig::DriveMotor::REVERSE:     eff = -eff;      break;
        case HardwareConfig::DriveMotor::UNI_FORWARD: eff = abs(eff);  break;
        case HardwareConfig::DriveMotor::UNI_REVERSE: eff = -abs(eff); break;
        default: break;
    }

    uint8_t pct = 0;
    if (eff > 0) {
        pct = ch.dutyMin + (uint8_t)((uint32_t)eff * (ch.dutyMax - ch.dutyMin) / 100);
    } else if (eff < 0) {
        pct = ch.dutyMin + (uint8_t)((uint32_t)(-eff) * (ch.dutyMax - ch.dutyMin) / 100);
    }
    if (pct > ch.dutyMax) pct = ch.dutyMax;

    if (ch.type == HardwareConfig::DriveMotor::ESC) {
        uint16_t us = 1500;
        if (abs(eff) >= 5) us = (uint16_t)(1500 + (int32_t)eff * 500 / 100);
        if (us < 1000) us = 1000;
        if (us > 2000) us = 2000;
        if (ch.esc) ch.esc->writeMicroseconds(us);
        return;
    }

    if (ch.driver) ch.driver->write(eff >= 0 ? (float)pct : -(float)pct);
}

void HardwareInit::setServo(int16_t position) {
    if (s_steeringServoCount == 0) return;
    if (position == s_lastServoPos) return;
    s_lastServoPos = position;

    for (uint8_t i = 0; i < s_steeringServoCount; i++) {
        if (!steeringServos[i].attached()) continue;
        const auto& ep = s_steeringConfigs[i].endpoints;
        int32_t us = ep.center;
        if (position > 0)      us = ep.center + (int32_t)position * (ep.right - ep.center) / 100;
        else if (position < 0) us = ep.center + (int32_t)position * (ep.center - ep.left) / 100;
        if (us < 500)  us = 500;
        if (us > 2500) us = 2500;

        steeringServos[i].writeMicroseconds(us);
    }
}

void HardwareInit::detachServos() {
    for (uint8_t i = 0; i < s_steeringServoCount; i++) {
        if (steeringServos[i].attached()) {
            steeringServos[i].detach();
        }
    }
    s_lastServoPos = -999;
}

void HardwareInit::attachServos() {
    for (uint8_t i = 0; i < s_steeringServoCount; i++) {
        if (!steeringServos[i].attached() && s_steeringConfigs[i].configured) {
            const auto& servo = s_steeringConfigs[i];
            EasyKit::ServoConfig cfg;
            cfg.minUs = 500;
            cfg.maxUs = 2500;
            cfg.centerUs = servo.endpoints.center;
            cfg.freq = (servo.frequency >= 40 && servo.frequency <= 400) ? servo.frequency : 50;
            steeringServos[i].attach(servo.hardwareId, cfg);
            steeringServos[i].writeMicroseconds(servo.endpoints.center);
        }
    }
    s_lastServoPos = -999;
}

void HardwareInit::setPump(bool active) {
    if (POWER::PUMP_ENABLE == 0xFF) return;
    pinMode(POWER::PUMP_ENABLE, OUTPUT);
    digitalWrite(POWER::PUMP_ENABLE, active ? HIGH : LOW);
}

void HardwareInit::setLight(uint8_t pin, uint8_t brightnessPct) {
    if (pin == 0xFF || pin == 0) return;
    EasyLED* led = findLight(pin);
    if (led) {
        led->write((float)brightnessPct);
    } else {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, brightnessPct > 0 ? HIGH : LOW);
    }
}

void HardwareInit::setLight(const HardwareConfig::Lights::Light& light, uint8_t brightnessPct) {
    if (!light.configured) return;
    for (uint8_t i = 0; i < light.pinCount; i++) {
        setLight(light.pins[i], brightnessPct);
    }
}

void HardwareInit::setLightBlink(uint8_t pin, bool active, uint16_t onMs, uint16_t offMs, uint8_t dutyPct) {
    if (pin == 0xFF || pin == 0) return;
    EasyLED* led = findLight(pin);
    if (!led) return;

    int8_t slot = blinkSlot(pin);
    if (slot < 0) return;

    if (active && !s_blinkActive[slot]) {
        s_blinkActive[slot] = true;
        led->startBlink(onMs, offMs, (float)dutyPct);
    } else if (!active && s_blinkActive[slot]) {
        s_blinkActive[slot] = false;
        led->stopBlink();
    }
}

void HardwareInit::setLightFade(uint8_t pin, uint8_t targetPct, uint16_t durationMs) {
    if (pin == 0xFF || pin == 0) return;
    EasyLED* led = findLight(pin);
    if (led) {
        uint32_t targetTicks = (uint32_t)led->getMaxDuty() * targetPct / 100;
        led->fadeTo(targetTicks, durationMs, EasyLED::Curve::EASE_IN_OUT);
    } else {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, targetPct > 0 ? HIGH : LOW);
    }
}

void HardwareInit::setLightFade(const HardwareConfig::Lights::Light& light, uint8_t targetPct, uint16_t durationMs) {
    if (!light.configured) return;
    for (uint8_t i = 0; i < light.pinCount; i++) {
        setLightFade(light.pins[i], targetPct, durationMs);
    }
}

void HardwareInit::stopLightAnimations() {
    for (uint8_t p = 0; p < headPinCount; p++) headLeds[p].stop();
    for (uint8_t p = 0; p < fullPinCount; p++) fullLeds[p].stop();
    for (uint8_t p = 0; p < fogPinCount; p++) fogLeds[p].stop();
    for (uint8_t p = 0; p < tailPinCount; p++) tailLeds[p].stop();
    for (uint8_t p = 0; p < brakePinCount; p++) brakeLeds[p].stop();
    for (uint8_t p = 0; p < reversingPinCount; p++) reversingLeds[p].stop();
    for (uint8_t p = 0; p < beaconPinCount; p++) beaconLeds[p].stop();
    for (uint8_t p = 0; p < cabPinCount; p++) cabLeds[p].stop();
    for (uint8_t p = 0; p < workPinCount; p++) workLeds[p].stop();
    for (uint8_t p = 0; p < stepPinCount; p++) stepLeds[p].stop();
    for (uint8_t p = 0; p < auxPinCount; p++) auxLeds[p].stop();
    turnLLed.stop();
    turnRLed.stop();
    ditchLLed.stop();
    ditchRLed.stop();
    s_ditchGroup.stop();
    s_ditchActive = false;
    resetBlinkTracking();
}

void HardwareInit::setDitchLights(bool active, uint16_t intervalMs) {
    if (ditchLPin == 0xFF && ditchRPin == 0xFF) return;
    if (active && !s_ditchActive) {
        s_ditchActive = true;
        s_ditchGroup.alternate(intervalMs);
    } else if (!active && s_ditchActive) {
        s_ditchActive = false;
        s_ditchGroup.stop();
    }
}

void HardwareInit::setAuxMotor(int16_t speed) {
    if (s_auxMotorCount == 0) return;
    for (uint8_t i = 0; i < s_auxMotorCount; i++) {
        const auto& aux = s_auxConfigs[i];
        if (aux.purpose == HardwareConfig::AuxMotor::NONE) continue;
        int16_t eff = speed;
        switch (aux.motor.direction) {
            case HardwareConfig::DriveMotor::REVERSE:     eff = -eff;      break;
            case HardwareConfig::DriveMotor::UNI_FORWARD: eff = abs(eff);  break;
            case HardwareConfig::DriveMotor::UNI_REVERSE: eff = -abs(eff); break;
            default: break;
        }

        uint8_t pct = 0;
        if (eff > 0) {
            pct = aux.motor.duty.min + (uint8_t)((uint32_t)eff * (aux.motor.duty.max - aux.motor.duty.min) / 100);
        } else if (eff < 0) {
            pct = aux.motor.duty.min + (uint8_t)((uint32_t)(-eff) * (aux.motor.duty.max - aux.motor.duty.min) / 100);
        }
        if (pct > aux.motor.duty.max) pct = aux.motor.duty.max;

        if (aux.motor.type == HardwareConfig::DriveMotor::ESC) {
            uint16_t us = 1500;
            if (abs(eff) >= 5) us = (uint16_t)(1500 + (int32_t)eff * 500 / 100);
            if (us < 1000) us = 1000;
            if (us > 2000) us = 2000;
            if (i < HardwareConfig::MAX_AUX_MOTORS) {
                auxServos[i].writeMicroseconds(us);
            }
        } else if (aux.motor.type == HardwareConfig::DriveMotor::DRIVER) {
            EasyMotor* drv = getDriverForId(aux.motor.hardwareId);
            if (drv) {
                drv->write(eff >= 0 ? (float)pct : -(float)pct);
            }
        }
    }
}

void HardwareInit::setAuxLight(uint8_t brightnessPct) {
    for (uint8_t p = 0; p < auxPinCount; p++) {
        setLight(auxPins[p], brightnessPct);
    }
}

void HardwareInit::setBeacon(bool active, uint16_t intervalMs) {
    for (uint8_t p = 0; p < beaconPinCount; p++) {
        setLightBlink(beaconPins[p], active, intervalMs, intervalMs, 100);
    }
}

uint8_t HardwareInit::getConfiguredLightMask(const HardwareConfig::Lights& L, bool isLoco) {
    uint8_t mask = 0;
    if (!isLoco) {
        if (L.headLight.configured) mask |= (1 << 0);
        if (L.fullBeam.configured || L.headLight.configured) mask |= (1 << 1);
        if (L.fogLamp.configured) mask |= (1 << 2);
        if (L.turnLight.configured) mask |= (1 << 3);
        if (L.beacon.configured) mask |= (1 << 4);
        if (L.cabLight.configured) mask |= (1 << 5);
        if (L.workLight.configured) mask |= (1 << 6);
        if (L.auxLight.configured || auxPinCount > 0) mask |= (1 << 7);
    } else {
        if (L.headLight.configured || L.tailLight.configured) mask |= (1 << 0);
        if (L.ditchLight.configured) mask |= (1 << 1);
        if (L.cabLight.configured) mask |= (1 << 2);
        if (L.stepLight.configured) mask |= (1 << 3);
        if (L.beacon.configured || L.fogLamp.configured || L.fullBeam.configured) mask |= (1 << 4);
        if (L.auxLight.configured || L.workLight.configured || auxPinCount > 0) mask |= (1 << 5);
    }
    return mask;
}

void HardwareInit::initAuxOutputs(const HardwareConfig& hw) {
    if (hw.drivetrainType == HardwareConfig::SKID_STEER) {
        s_auxMotorCount = 0;
        auxPinCount = 0;
        return;
    }
    uint8_t effAuxCount = hw.auxMotorCount;
    if (effAuxCount == 0 && hw.auxMotors[0].configured) {
        effAuxCount = 1;
    }
    s_auxMotorCount = effAuxCount;
    for (uint8_t i = 0; i < effAuxCount && i < HardwareConfig::MAX_AUX_MOTORS; i++) {
        s_auxConfigs[i] = hw.auxMotors[i];
        const auto& aux = hw.auxMotors[i];
        if (aux.motor.type == HardwareConfig::DriveMotor::DRIVER) {
            const char* name = getDriverName(aux.motor.hardwareId);
            DriverPins pins = PinMapper::getDriver(name);
            EasyMotor* drv = getDriverForId(aux.motor.hardwareId);
            if (drv) {
                if (pins.dualPwm) {
                    drv->begin(EasyMotor::DriverType::DRIVER_2PWM,
                               pins.pwm1, pins.pwm2, pins.enable, false);
                } else {
                    drv->begin(EasyMotor::DriverType::DRIVER_1PWM_1DIR,
                               pins.pwm1, pins.pwm2, pins.enable, true);
                }
                drv->setFrequency(aux.motor.frequency);
            }
        } else if (aux.motor.type == HardwareConfig::DriveMotor::ESC) {
            EasyKit::ServoConfig cfg;
            cfg.minUs = 1000;
            cfg.maxUs = 2000;
            cfg.centerUs = 1500;
            cfg.freq = (aux.motor.frequency >= 40 && aux.motor.frequency <= 900)
                           ? (uint16_t)aux.motor.frequency : 50;
            if (i < HardwareConfig::MAX_AUX_MOTORS) {
                auxServos[i].attach(aux.motor.hardwareId, cfg);
            }
        }
    }

    if (hw.auxLight.configured) {
        auxPinCount = 0;
        uint8_t effectiveCount = hw.auxLight.pinCount;
        if (effectiveCount == 0 && hw.auxLight.pin != 0xFF) effectiveCount = 1;
        const EasyKit::LEDConfig cfg = {5000, EasyKit::LEDCResolution::Bits10, -1, false};
        for (uint8_t p = 0; p < effectiveCount && p < HardwareConfig::MAX_PINS_PER_LIGHT; p++) {
            auxPins[p] = (p < hw.auxLight.pinCount) ? hw.auxLight.pins[p] : hw.auxLight.pin;
            auxLeds[p].begin(auxPins[p], cfg);
            auxPinCount++;
        }
    }
}

EasyLED* HardwareInit::findLight(uint8_t pin) {
    for (uint8_t p = 0; p < headPinCount; p++) if (pin == headPins[p] && headLeds[p].isAttached()) return &headLeds[p];
    for (uint8_t p = 0; p < fullPinCount; p++) if (pin == fullPins[p] && fullLeds[p].isAttached()) return &fullLeds[p];
    for (uint8_t p = 0; p < fogPinCount; p++) if (pin == fogPins[p] && fogLeds[p].isAttached()) return &fogLeds[p];
    for (uint8_t p = 0; p < tailPinCount; p++) if (pin == tailPins[p] && tailLeds[p].isAttached()) return &tailLeds[p];
    for (uint8_t p = 0; p < brakePinCount; p++) if (pin == brakePins[p] && brakeLeds[p].isAttached()) return &brakeLeds[p];
    for (uint8_t p = 0; p < reversingPinCount; p++) if (pin == reversingPins[p] && reversingLeds[p].isAttached()) return &reversingLeds[p];
    for (uint8_t p = 0; p < beaconPinCount; p++) if (pin == beaconPins[p] && beaconLeds[p].isAttached()) return &beaconLeds[p];
    for (uint8_t p = 0; p < cabPinCount; p++) if (pin == cabPins[p] && cabLeds[p].isAttached()) return &cabLeds[p];
    for (uint8_t p = 0; p < workPinCount; p++) if (pin == workPins[p] && workLeds[p].isAttached()) return &workLeds[p];
    for (uint8_t p = 0; p < stepPinCount; p++) if (pin == stepPins[p] && stepLeds[p].isAttached()) return &stepLeds[p];
    for (uint8_t p = 0; p < auxPinCount; p++) if (pin == auxPins[p] && auxLeds[p].isAttached()) return &auxLeds[p];

    if (pin == turnLPin && turnLLed.isAttached()) return &turnLLed;
    if (pin == turnRPin && turnRLed.isAttached()) return &turnRLed;
    if (pin == ditchLPin && ditchLLed.isAttached()) return &ditchLLed;
    if (pin == ditchRPin && ditchRLed.isAttached()) return &ditchRLed;
    return nullptr;
}

void HardwareInit::resetBlinkTracking() {
    for (int i = 0; i < 4; i++) {
        s_blinkPin[i] = 0xFF;
        s_blinkActive[i] = false;
    }
}

int8_t HardwareInit::blinkSlot(uint8_t pin) {
    for (int i = 0; i < 4; i++) {
        if (s_blinkPin[i] == pin) return i;
        if (s_blinkPin[i] == 0xFF) {
            s_blinkPin[i] = pin;
            s_blinkActive[i] = false;
            return i;
        }
    }
    return -1;
}

void HardwareInit::initChannel(MotorChannel& ch, const HardwareConfig::DriveMotor& motor,
                        EasyMotor* driver, EasyServo* esc) {
    if (motor.type == HardwareConfig::DriveMotor::NONE) {
        ch.type = HardwareConfig::DriveMotor::NONE;
        ch.attached = false;
        return;
    }

    ch.type = motor.type;
    ch.direction = motor.direction;
    ch.dutyMin = motor.duty.min;
    ch.dutyMax = motor.duty.max;
    ch.frequency = motor.frequency;
    ch.driver = driver;
    ch.esc = esc;

    if (motor.type == HardwareConfig::DriveMotor::DRIVER) {
        const char* name = getDriverName(motor.hardwareId);
        DriverPins pins = PinMapper::getDriver(name);
        ch.bemfPin = pins.bemf;

        if (driver) {
            if (pins.dualPwm) {
                driver->begin(EasyMotor::DriverType::DRIVER_2PWM,
                              pins.pwm1, pins.pwm2, pins.enable, false);
            } else {
                driver->begin(EasyMotor::DriverType::DRIVER_1PWM_1DIR,
                              pins.pwm1, pins.pwm2, pins.enable, true);
            }
            driver->setFrequency(motor.frequency);
            ch.attached = true;
            Serial.printf("[HardwareInit] Motor (driver): %s Freq=%dHz Duty=%d..%d%%\n",
                          name, motor.frequency, motor.duty.min, motor.duty.max);
        }
    }
    else if (motor.type == HardwareConfig::DriveMotor::ESC) {
        EasyKit::ServoConfig cfg;
        cfg.minUs = 1000;
        cfg.maxUs = 2000;
        cfg.centerUs = 1500;
        cfg.freq = (motor.frequency >= 40 && motor.frequency <= 900)
                       ? (uint16_t)motor.frequency : 50;
        if (esc->attach(motor.hardwareId, cfg) == EasyKit::Result::OK) {
            ch.attached = true;
            Serial.printf("[HardwareInit] Motor (servo/ESC): Pin=%d Freq=%dHz Duty=%d..%d%%\n",
                          motor.hardwareId, cfg.freq, motor.duty.min, motor.duty.max);
        } else {
            ch.attached = false;
            Serial.printf("[HardwareInit] Motor ESC attach FAILED on Pin=%d\n",
                          motor.hardwareId);
        }
    }
}

void HardwareInit::initSteeringServos(const HardwareConfig& hw) {
    s_steeringServoCount = 0;
    s_lastServoPos = -999;
    uint8_t effSteeringCount = hw.steeringServoCount;
    if (effSteeringCount == 0 && hw.steeringServos[0].configured) {
        effSteeringCount = 1;
    }
    for (uint8_t i = 0; i < effSteeringCount && i < HardwareConfig::MAX_STEERING_SERVOS; i++) {
        const auto& servo = hw.steeringServos[i];
        if (!servo.configured) continue;
        s_steeringConfigs[i] = servo;

        EasyKit::ServoConfig cfg;
        cfg.minUs = 500;
        cfg.maxUs = 2500;
        cfg.centerUs = servo.endpoints.center;
        cfg.freq = (servo.frequency >= 40 && servo.frequency <= 400) ? servo.frequency : 50;

        if (steeringServos[i].attach(servo.hardwareId, cfg) == EasyKit::Result::OK) {
            s_steeringServoCount++;
            Serial.printf("[HardwareInit] Steering servo[%d]: Pin=%d Freq=%dHz Center=%dus (L=%d, R=%d)\n",
                          i, servo.hardwareId, cfg.freq, servo.endpoints.center, servo.endpoints.left, servo.endpoints.right);
            steeringServos[i].writeMicroseconds(servo.endpoints.center);
        } else {
            Serial.printf("[HardwareInit] Steering servo[%d] attach FAILED on Pin=%d\n", i, servo.hardwareId);
        }
    }
}

void HardwareInit::initLights(const HardwareConfig::Lights& lights) {
    headPinCount = 0; fullPinCount = 0; fogPinCount = 0;
    tailPinCount = 0; brakePinCount = 0; reversingPinCount = 0;
    beaconPinCount = 0; cabPinCount = 0; workPinCount = 0;
    stepPinCount = 0; auxPinCount = 0;
    turnLPin = 0xFF; turnRPin = 0xFF; ditchLPin = 0xFF; ditchRPin = 0xFF;

    const EasyKit::LEDConfig cfg = {5000, EasyKit::LEDCResolution::Bits10, -1, false};

    auto initGroup = [&](const HardwareConfig::Lights::Light& light, EasyLED* leds, uint8_t* pins, uint8_t& count, const char* name) {
        count = 0;
        if (!light.configured) return;
        uint8_t effectiveCount = light.pinCount;
        if (effectiveCount == 0 && light.pin != 0xFF) effectiveCount = 1;
        for (uint8_t p = 0; p < effectiveCount && p < HardwareConfig::MAX_PINS_PER_LIGHT; p++) {
            pins[p] = (p < light.pinCount) ? light.pins[p] : light.pin;
            leds[p].begin(pins[p], cfg);
            count++;
            Serial.printf("[HardwareInit] %s[%d]: Pin=%d Brightness=%d%%\n", name, p, pins[p], light.brightness);
        }
    };

    initGroup(lights.headLight, headLeds, headPins, headPinCount, "Headlight");
    initGroup(lights.fullBeam, fullLeds, fullPins, fullPinCount, "Full beam");
    initGroup(lights.fogLamp, fogLeds, fogPins, fogPinCount, "Fog lamp");
    initGroup(lights.tailLight, tailLeds, tailPins, tailPinCount, "Taillight");
    initGroup(lights.brakeLight, brakeLeds, brakePins, brakePinCount, "Brakelight");
    initGroup(lights.beacon, beaconLeds, beaconPins, beaconPinCount, "Beacon");
    initGroup(lights.cabLight, cabLeds, cabPins, cabPinCount, "Cab light");
    initGroup(lights.workLight, workLeds, workPins, workPinCount, "Work light");
    initGroup(lights.stepLight, stepLeds, stepPins, stepPinCount, "Step light");
    initGroup(lights.auxLight, auxLeds, auxPins, auxPinCount, "Aux light");

    if (lights.reversingLight.configured) {
        reversingPinCount = 0;
        uint8_t effectiveCount = lights.reversingLight.pinCount;
        if (effectiveCount == 0 && lights.reversingLight.pin != 0xFF) effectiveCount = 1;
        for (uint8_t p = 0; p < effectiveCount && p < HardwareConfig::MAX_PINS_PER_LIGHT; p++) {
            uint8_t pin = (p < lights.reversingLight.pinCount) ? lights.reversingLight.pins[p] : lights.reversingLight.pin;
            reversingPins[p] = pin;
            if (!findLight(pin)) {
                reversingLeds[p].begin(pin, cfg);
            }
            reversingPinCount++;
            Serial.printf("[HardwareInit] Reversing light[%d]: Pin=%d\n", p, pin);
        }
    }

    if (lights.ditchLight.configured) {
        ditchLPin = lights.ditchLight.leftPin;
        ditchRPin = lights.ditchLight.rightPin;
        s_ditchBrightness = lights.ditchLight.brightness;
        if (ditchLPin != 0xFF) ditchLLed.begin(ditchLPin, cfg);
        if (ditchRPin != 0xFF) ditchRLed.begin(ditchRPin, cfg);
        s_ditchGroup.clearMembers();
        if (ditchLPin != 0xFF) s_ditchGroup.addMember(&ditchLLed);
        if (ditchRPin != 0xFF) s_ditchGroup.addMember(&ditchRLed);
        s_ditchActive = false;
        Serial.printf("[HardwareInit] Ditch lights: L=%d R=%d Interval=%dms Brightness=%d%%\n",
                      ditchLPin, ditchRPin, lights.ditchLight.intervalMs,
                      lights.ditchLight.brightness);
    }

    if (lights.turnLight.configured) {
        turnLPin = lights.turnLight.leftPin;
        turnRPin = lights.turnLight.rightPin;
        if (turnLPin != 0xFF) turnLLed.begin(turnLPin, cfg);
        if (turnRPin != 0xFF) turnRLed.begin(turnRPin, cfg);
        Serial.printf("[HardwareInit] Turn signals: L=%d R=%d Interval=%d/%dms\n",
                      turnLPin, turnRPin,
                      lights.turnLight.intervalOn, lights.turnLight.intervalOff);
    }
}
