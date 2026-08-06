/**
 * @file ledc_manager.cpp
 * @brief LEDC channel allocator implementation with multi-pin/sharing support.
 */

#include "ledc_manager.h"
#include <cstring>
#include <algorithm>

namespace EasyKit {

LEDCManager::LEDCManager() {
    _mutex = xSemaphoreCreateMutex();
    for (uint8_t i = 0; i < totalChannels(); ++i) {
        _channels[i].pinMask = 0;
        _channels[i].inUse   = false;
    }
}

LEDCManager& LEDCManager::instance() {
    static LEDCManager inst;
    return inst;
}

int8_t LEDCManager::allocate(uint8_t pin, uint32_t freq, uint8_t resolution) {
    if (pin >= 64) return -1; // Protect against invalid shifts
    int8_t result = -1;
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        // First check if this pin is already registered somewhere
        for (uint8_t i = 0; i < totalChannels(); ++i) {
            if (_channels[i].pinMask & (1ULL << pin)) {
                result = i;
                goto end;
            }
        }

        // Find a free channel
        for (uint8_t i = 0; i < totalChannels(); ++i) {
            if (!_channels[i].inUse) {
                _channels[i].pinMask    |= (1ULL << pin);
                _channels[i].freq       = freq;
                _channels[i].resolution = resolution;
                _channels[i].inUse      = true;
                result = i;
                break;
            }
        }
end:
        xSemaphoreGive(_mutex);
    }
    return result;
}

int8_t LEDCManager::allocateChannel(uint8_t pin, uint32_t freq, uint8_t resolution, int8_t channel) {
    if (channel < 0 || channel >= totalChannels() || pin >= 64) return -1;

    int8_t result = -1;
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        // If channel is already in use, we allow joining if it's the same frequency/resolution
        if (_channels[channel].inUse) {
            // Check if pin is already in THIS channel
            if (_channels[channel].pinMask & (1ULL << pin)) {
                result = channel;
            } else if (_channels[channel].freq == freq && _channels[channel].resolution == resolution) {
                _channels[channel].pinMask |= (1ULL << pin);
                result = channel;
            }
        } else {
            // Fresh allocation
            _channels[channel].pinMask    |= (1ULL << pin);
            _channels[channel].freq       = freq;
            _channels[channel].resolution = resolution;
            _channels[channel].inUse      = true;
            result = channel;
        }
        xSemaphoreGive(_mutex);
    }
    return result;
}

void LEDCManager::release(uint8_t pin) {
    if (pin >= 64) return;
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        for (uint8_t i = 0; i < totalChannels(); ++i) {
            if (_channels[i].pinMask & (1ULL << pin)) {
                _channels[i].pinMask &= ~(1ULL << pin);
                if (_channels[i].pinMask == 0) {
                    _channels[i].inUse = false;
                }
                break; // A pin is only in one channel
            }
        }
        xSemaphoreGive(_mutex);
    }
}

uint8_t LEDCManager::numFree() const {
    uint8_t count = 0;
    for (uint8_t i = 0; i < totalChannels(); ++i) {
        if (!_channels[i].inUse) ++count;
    }
    return count;
}

} // namespace EasyKit
