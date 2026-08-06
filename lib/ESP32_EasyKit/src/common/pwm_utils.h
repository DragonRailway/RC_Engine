/**
 * @file pwm_utils.h
 * @brief Duty-cycle math helpers for ESP32_EasyKit.
 */
#pragma once

#include <cstdint>
#include "pwm_types.h"

namespace EasyKit {

/// Convert an 8-bit value (0–255) to a raw duty value for the given resolution.
inline uint32_t analogToRaw(uint8_t value, uint8_t resolution) {
    uint32_t maxDuty = (1UL << resolution) - 1;
    return (uint32_t)((float)value / 255.0f * maxDuty + 0.5f);
}

/// Convert a raw duty value to an 8-bit value (0–255) for the given resolution.
inline uint8_t rawToAnalog(uint32_t raw, uint8_t resolution) {
    uint32_t maxDuty = (1UL << resolution) - 1;
    if (maxDuty == 0) return 0;
    return (uint8_t)((float)raw / maxDuty * 255.0f + 0.5f);
}

/// Convert a percentage duty (0–100) to raw duty for the given resolution.
inline uint32_t percentToRaw(uint8_t percent, uint8_t resolution) {
    if (percent == 0) return 0;
    if (percent >= 100) return (1UL << resolution) - 1;
    uint32_t maxDuty = (1UL << resolution) - 1;
    return (uint32_t)((float)percent / 100.0f * maxDuty + 0.5f);
}

/// Convert a percentage duty (float 0–100) to raw duty for the given resolution.
inline uint32_t percentToRaw(float percent, uint8_t resolution) {
    if (percent <= 0.0f) return 0;
    uint32_t maxDuty = (1UL << resolution) - 1;
    if (percent >= 100.0f) return maxDuty;
    return (uint32_t)(percent / 100.0f * (float)maxDuty + 0.5f);
}

/// Convert a raw duty to percentage (0–100) for the given resolution.
inline uint8_t rawToPercent(uint32_t raw, uint8_t resolution) {
    uint32_t maxDuty = (1UL << resolution) - 1;
    if (maxDuty == 0) return 0;
    return (uint8_t)((float)raw / maxDuty * 100.0f + 0.5f);
}

/// Convert a raw duty to percentage (float 0.0–100.0) for the given resolution.
inline float rawToPercentFloat(uint32_t raw, uint8_t resolution) {
    uint32_t maxDuty = (1UL << resolution) - 1;
    if (maxDuty == 0) return 0.0f;
    return (float)raw / (float)maxDuty * 100.0f;
}

/// Get the maximum duty value for a given resolution.
inline uint32_t maxDutyForResolution(uint8_t resolution) {
    return (1UL << resolution) - 1;
}

/// Linear interpolation: map value from [inMin, inMax] to [outMin, outMax].
inline int mapRange(int value, int inMin, int inMax, int outMin, int outMax) {
    return outMin + (int)((float)(value - inMin) / (inMax - inMin) * (outMax - outMin) + 0.5f);
}

/// Clamp a value between min and max.
template <typename T>
inline T clampValue(T value, T minVal, T maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

} // namespace EasyKit
