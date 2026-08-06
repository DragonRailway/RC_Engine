/**
 * @file mcpwm_manager.cpp
 * @brief MCPWM operator allocator implementation.
 */

#include "mcpwm_manager.h"

#ifdef SOC_MCPWM_SUPPORTED

namespace EasyKit {

MCPWMManager::MCPWMManager()
    : _used(0)
{
    _mutex = xSemaphoreCreateMutex();
}

MCPWMManager& MCPWMManager::instance() {
    static MCPWMManager inst;
    return inst;
}

MCPWMManager::Slot MCPWMManager::allocate() {
    Slot result = { -1, -1 };
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        for (uint8_t i = 0; i < totalSlots(); ++i) {
            if (!(_used & (1U << i))) {
                _used |= (1U << i);
                result.unit    = i / SOC_MCPWM_OPERATORS_PER_GROUP;
                result.opIndex = i % SOC_MCPWM_OPERATORS_PER_GROUP;
                break;
            }
        }
        xSemaphoreGive(_mutex);
    }
    return result;
}

void MCPWMManager::release(Slot s) {
    if (s.unit < 0 || s.opIndex < 0) return;
    uint8_t idx = s.unit * SOC_MCPWM_OPERATORS_PER_GROUP + s.opIndex;
    if (idx >= totalSlots()) return;

    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        _used &= ~(1U << idx);
        xSemaphoreGive(_mutex);
    }
}

uint8_t MCPWMManager::numFree() const {
    uint8_t count = 0;
    for (uint8_t i = 0; i < totalSlots(); ++i) {
        if (!(_used & (1U << i))) ++count;
    }
    return count;
}

} // namespace EasyKit

#endif // SOC_MCPWM_SUPPORTED
