/**
 * @file ESP32_EasyKit.h
 * @brief Master include for ESP32_EasyKit.
 *
 * Include this single header to access all library classes:
 *   - EasyLED   (LEDC-based, unified LED control + fading + blink patterns)
 *   - Servo       (MCPWM-based, Arduino Servo.h compatible)
 *   - EasyMotor     (MCPWM-based, H-bridge control)
 *
 *
 */
#pragma once

#define ESP32EASYKIT_VERSION_MAJOR 1
#define ESP32EASYKIT_VERSION_MINOR 1
#define ESP32EASYKIT_VERSION_PATCH 0

// Common types & utilities
#include "common/pwm_types.h"
#include "common/pwm_utils.h"

// MCPWM classes (conditionally compiled per SoC)
#include "EasyServo.h"
#include "EasyMotor.h"

// LEDC — EasyLED Class
#include "EasyLED.h"
