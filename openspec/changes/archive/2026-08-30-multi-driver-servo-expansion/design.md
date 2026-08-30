## Context

Previously, `HardwareInit` maintained only 2 hardcoded `EasyMotor` instances:
```cpp
static EasyMotor driveMotor;
static EasyMotor auxMotor;
```
And driver selection was a binary branch: `(hardwareId == PinMapper::DRIVER_B) ? &auxMotor : &driveMotor`.

This prevented boards with more than 2 drivers (or setups needing multiple aux drivers + drive motors) from functioning without collisions.

## Goals / Non-Goals

**Goals:**
- Provide a physical pool of 4 `EasyMotor` instances for `DRIVER_A`, `DRIVER_B`, `DRIVER_C`, and `DRIVER_D`.
- Provide a physical pool of 4 `EasyServo` instances across steering, aux actuators, and ESCs.
- Add `DRIVER_C` (`0xE3`) and `DRIVER_D` (`0xE4`) hardware IDs to `PinMapper.h`.
- Maintain clean, backward-compatible binding: any configured drive motor or aux motor can claim any driver `0xE1`..`0xE4`.

**Non-Goals:**
- Modifying vehicle physics models or adding complex 4WD independent differential algorithms (existing 2-channel drive throttle and 2-channel steering models remain intact).

## Decisions

### 1. Indexed Driver Pool in `HardwareInit`
Replace individual `driveMotor`/`auxMotor` instances with a 4-element array:
```cpp
static EasyMotor s_motorDrivers[4]; // Indices 0..3 for DRIVER_A..DRIVER_D
```
Helper function for resolution:
```cpp
static EasyMotor* getDriverForId(uint8_t hardwareId) {
    if (hardwareId >= PinMapper::DRIVER_A && hardwareId <= PinMapper::DRIVER_D) {
        return &s_motorDrivers[hardwareId - PinMapper::DRIVER_A];
    }
    return nullptr;
}
```

### 2. Indexed Servo Pool
Maintain 2 dedicated steering servos (`steeringServos[2]`), 2 dedicated aux servos (`auxServos[2]`), and 1 ESC drive servo (`escServo`), ensuring up to 4 concurrent physical servo attachments without memory overlap.

### 3. PinMapper Hardware IDs
Define consecutive IDs above standard GPIO range:
```cpp
static constexpr uint8_t DRIVER_A = 0xE1;
static constexpr uint8_t DRIVER_B = 0xE2;
static constexpr uint8_t DRIVER_C = 0xE3;
static constexpr uint8_t DRIVER_D = 0xE4;
```
Mapping:
```cpp
static uint8_t resolve(const char* name) {
    if (!name) return NO_PIN;
    if (strcmp(name, "DRIVER_A") == 0) return DRIVER_A;
    if (strcmp(name, "DRIVER_B") == 0) return DRIVER_B;
    if (strcmp(name, "DRIVER_C") == 0) return DRIVER_C;
    if (strcmp(name, "DRIVER_D") == 0) return DRIVER_D;
    return Board::resolve(name);
}
```

## Risks / Trade-offs

- **[Memory overhead]** → 4 `EasyMotor` instances add ~100 bytes of static RAM on ESP32-S3, which is negligible.
- **[Backwards compatibility]** → `PinMapper::DRIVER_A` and `DRIVER_B` retain their exact `0xE1` and `0xE2` values, guaranteeing zero breakage for existing configs and test suites.
