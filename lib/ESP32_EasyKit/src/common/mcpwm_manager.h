/**
 * @file mcpwm_manager.h
 * @brief Internal MCPWM operator/unit allocator (singleton).
 *
 * Tracks available MCPWM operator slots across all units.
 * Thread-safe via FreeRTOS mutex.
 */
#pragma once

#include <cstdint>

// Guard: only compile on chips that have MCPWM
#if __has_include("soc/soc_caps.h")
#include "soc/soc_caps.h"
#endif

#ifdef SOC_MCPWM_SUPPORTED

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace EasyKit {

class MCPWMManager {
public:
    struct Slot {
        int8_t unit;     ///< MCPWM group index (0 or 1)
        int8_t opIndex;  ///< Operator index within the unit (0–2)
    };

    /// Get the singleton instance.
    static MCPWMManager& instance();

    /// Allocate the next free operator slot.
    /// @return Slot with valid unit/opIndex on success, or {-1, -1} on failure.
    Slot allocate();

    /// Release a previously allocated slot.
    void release(Slot s);

    /// Number of free operator slots remaining.
    uint8_t numFree() const;

    /// Total number of operator slots on this SoC.
    static constexpr uint8_t totalSlots() {
        return SOC_MCPWM_GROUPS * SOC_MCPWM_OPERATORS_PER_GROUP;
    }

private:
    MCPWMManager();
    ~MCPWMManager() = default;
    MCPWMManager(const MCPWMManager&) = delete;
    MCPWMManager& operator=(const MCPWMManager&) = delete;

    SemaphoreHandle_t _mutex;
    uint8_t _used;  ///< Bitmask of allocated slots
};

} // namespace EasyKit

#endif // SOC_MCPWM_SUPPORTED
