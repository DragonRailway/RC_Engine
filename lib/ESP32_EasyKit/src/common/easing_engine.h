/**
 * @file easing_engine.h
 * @brief Mathematical Utility for ESP32_EasyKit.
 * 
 * Provides easing formulas (Sigmoid, Quadratic, Sine) with 
 * single-precision float operations for the ESP32 hardware FPU.
 * 
 * DESIGN RULES:
 * - Always use float, never double.
 * - All literals must be suffixed with 'f'.
 * - Use sinf, cosf, fabsf, etc.
 */

#pragma once

#include <cmath>

namespace EasyKit {

/**
 * @brief Mathematical collection of easing formulas.
 * Designed for ESP32 Hardware FPU.
 */
class Easing {
public:
    /**
     * @brief Perfectly linear progress.
     */
    static inline float linear(float t) {
        return t;
    }

    /**
     * @brief Classic Quadratic Ease-In (Slow start, fast finish).
     * formula: f(t) = t^2
     */
    static inline float quadraticIn(float t) {
        return t * t;
    }

    /**
     * @brief Classic Quadratic Ease-Out (Fast start, slow finish).
     * formula: f(t) = 1 - (1-t)^2
     */
    static inline float quadraticOut(float t) {
        float inv = 1.0f - t;
        return 1.0f - (inv * inv);
    }

    /**
     * @brief Smooth Sine S-curve.
     */
    static inline float sine(float t) {
        // Use PI literal with 'f' suffix
        const float PI_F = 3.14159265f;
        return (1.0f - cosf(t * PI_F)) / 2.0f;
    }

    /**
     * @brief Parametric Asymmetric Sigmoid (Tunable S-curve).
     * 
     * @param t Progress (0.0f to 1.0f)
     * @param kIn Easing strength at start (0.0f to 0.99f)
     * @param kOut Easing strength at end (0.0f to 0.99f)
     * @return float Eased progress
     */
    static inline float sigmoid(float t, float kIn, float kOut) {
        if (t <= 0.0f) return 0.0f;
        if (t >= 1.0f) return 1.0f;

        if (t < 0.5f) {
            // First half
            float x = 2.0f * t;
            // f(x, k) = (x - kx) / (k - 2kx + 1)
            float num = x - (kIn * x);
            float den = kIn - (2.0f * kIn * x) + 1.0f;
            return 0.5f * (num / den);
        } else {
            // Second half
            float x = 2.0f * (1.0f - t);
            float num = x - (kOut * x);
            float den = kOut - (2.0f * kOut * x) + 1.0f;
            return 1.0f - (0.5f * (num / den));
        }
    }

    /**
     * @brief Symmetric Sigmoid helper.
     */
    static inline float sigmoid(float t, float k) {
        return sigmoid(t, k, k);
    }
};

} // namespace EasyKit
