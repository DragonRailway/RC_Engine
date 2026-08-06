/**
 * @file pin_manager.cpp
 * @brief Implementation of GlobalPinManager with Periman integration.
 */

#include "pin_manager.h"
#include <Arduino.h>

namespace EasyKit {

IPinOwner* GlobalPinManager::_owners[64] = {nullptr};

void GlobalPinManager::claimPin(uint8_t pin, IPinOwner* newOwner) {
    if (pin >= 64 || newOwner == nullptr) return;

    // 1. Check for internal library conflicts
    if (_owners[pin] != nullptr && _owners[pin] != newOwner) {
        // The pin is currently owned by another library object. Notify them.
        _owners[pin]->onPinStolen(pin);
    }
    
    // 2. Arduino Core Periman Integration
#ifdef HAS_PERIMAN
    // IMPORTANT: We use ESP32_BUS_TYPE_GPIO (Type 2) to signify generic use.
    // My previous use of ESP32_BUS_TYPE_MAX caused conflicts with the PPP Modem driver.
    perimanSetPinBus(pin, ESP32_BUS_TYPE_GPIO, (void*)newOwner, -1, -1);
    perimanSetPinBusExtraType(pin, newOwner->getPeripheralName());
#endif

    // 3. Register the new owner internally
    _owners[pin] = newOwner;
}

void GlobalPinManager::releasePin(uint8_t pin, IPinOwner* currentOwner) {
    if (pin >= 64 || currentOwner == nullptr) return;

    if (_owners[pin] == currentOwner) {
        _owners[pin] = nullptr;
#ifdef HAS_PERIMAN
        perimanClearPinBus(pin);
#endif
    }
}

IPinOwner* GlobalPinManager::getOwner(uint8_t pin) {
    if (pin >= 64) return nullptr;
    return _owners[pin];
}

} // namespace EasyKit
