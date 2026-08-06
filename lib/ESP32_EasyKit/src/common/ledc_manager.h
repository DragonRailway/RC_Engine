/**
 * @file ledc_manager.h
 * @brief Internal LEDC channel allocator (singleton).
 *
 * Tracks available LEDC channels. Thread-safe via FreeRTOS mutex.
 */
#pragma once

#include <cstdint>

#if __has_include("soc/soc_caps.h")
#include "soc/soc_caps.h"
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifndef SOC_LEDC_CHANNEL_NUM
#define SOC_LEDC_CHANNEL_NUM 8  // safe default
#endif

#include <cstdint>

namespace EasyKit {

class LEDCManager {
public:
    /// Get the singleton instance.
    static LEDCManager& instance();

    /// Allocate a free LEDC channel for a given pin/freq/resolution.
    int8_t allocate(uint8_t pin, uint32_t freq, uint8_t resolution);

    /// Allocate/Join a specific LEDC channel for a pin.
    int8_t allocateChannel(uint8_t pin, uint32_t freq, uint8_t resolution, int8_t channel);

    /// Release a pin from its channel. If it was the last pin, the channel is freed.
    void release(uint8_t pin);

    /// Number of free channels remaining.
    uint8_t numFree() const;

    /// Total number of channels on this SoC.
    static constexpr uint8_t totalChannels() {
        return SOC_LEDC_CHANNEL_NUM;
    }

private:
    LEDCManager();
    ~LEDCManager() = default;
    LEDCManager(const LEDCManager&) = delete;
    LEDCManager& operator=(const LEDCManager&) = delete;

    struct ChannelInfo {
        uint64_t pinMask;
        uint32_t freq;
        uint8_t  resolution;
        bool     inUse;
    };

    SemaphoreHandle_t _mutex;
    ChannelInfo _channels[SOC_LEDC_CHANNEL_NUM];
};

} // namespace EasyKit

