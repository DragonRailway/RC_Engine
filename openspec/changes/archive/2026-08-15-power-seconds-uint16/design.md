# Design: uint16_t Power Time Parameters

## Data Flow & Architecture

In `common/Config.h`:
```cpp
struct Power {
    uint8_t  indicatorPin = 0xFF;
    uint16_t bootLatchS = 1;
    uint16_t buttonHoldS = 4;
    uint16_t disconnectTimeoutS = 60;
    uint16_t warningWindowS = 10;
    uint16_t cutoffDelayS = 2;
} power;
```

In `common/HardwareInit.h` and `common/VehicleController.h`, duration in milliseconds is computed via unsigned integer multiplication:
```cpp
uint32_t holdMs = (uint32_t)buttonHoldS * 1000U;
```
This avoids floating-point operations in timing loops.
