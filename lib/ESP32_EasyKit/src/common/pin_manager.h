#pragma once

#include <cstdint>

#if __has_include("esp32-hal-periman.h")
#include "esp32-hal-periman.h"
#define HAS_PERIMAN
#endif

namespace EasyKit {

/**
 * @brief Interface for objects that can "own" a GPIO pin.
 */
class IPinOwner {
public:
    virtual ~IPinOwner() = default;
    
    /**
     * @brief Called when another component claims a pin previously owned by this object.
     * The object should immediately stop driving the pin and release its hardware resources.
     */
    virtual void onPinStolen(uint8_t pin) = 0;

    /**
     * @brief Get a human-readable name for the peripheral type (e.g., "MCPWM", "LEDC").
     */
    virtual const char* getPeripheralName() const { return "PWM_FUSION"; }
};

/**
 * @brief Global tracker for GPIO pins across MCPWM and LEDC.
 */
class GlobalPinManager {
public:
    /**
     * @brief Claim a pin for a specific owner. 
     * If the pin is already owned, the previous owner is notified via onPinStolen().
     */
    static void claimPin(uint8_t pin, IPinOwner* newOwner);

    /**
     * @brief Manually release a pin. 
     */
    static void releasePin(uint8_t pin, IPinOwner* currentOwner);

    /**
     * @brief Check who currently owns a pin.
     */
    static IPinOwner* getOwner(uint8_t pin);

private:
    static IPinOwner* _owners[64];
};

} // namespace EasyKit
