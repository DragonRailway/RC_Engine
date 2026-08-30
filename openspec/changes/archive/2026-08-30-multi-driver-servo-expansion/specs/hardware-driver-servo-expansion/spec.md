## ADDED Requirements

### Requirement: 4-Slot physical H-bridge driver pool
`HardwareInit` SHALL manage a physical pool of up to 4 `EasyMotor` hardware instances corresponding to `DRIVER_A`, `DRIVER_B`, `DRIVER_C`, and `DRIVER_D`. Any configured drive motor, auxiliary motor, or trailer output requesting a driver hardware token SHALL claim its corresponding `EasyMotor` instance from this pool.

#### Scenario: Driver A and Driver B drive motors with Driver C and Driver D aux motors
- **WHEN** a hardware configuration assigns `DRIVER_A` and `DRIVER_B` to drive motors, and `DRIVER_C` and `DRIVER_D` to aux motors
- **THEN** all 4 `EasyMotor` instances are initialized with their respective pin mappings and PWM frequencies without resource collisions

#### Scenario: Stopping all active drivers
- **WHEN** `HardwareInit::stopAll()` is called
- **THEN** all 4 motor driver channels are commanded to stop (zero speed)

---

### Requirement: 4-Slot physical servo/ESC output pool
`HardwareInit` SHALL manage a physical pool of up to 4 `EasyServo` hardware instances claimable across steering servos, auxiliary servos, and ESC-driven motors.

#### Scenario: Concurrent steering servos and aux servos
- **WHEN** a hardware configuration configures 2 steering servos and 2 auxiliary servos on physical servo pins (`S1`–`S4` or `E1`–`E4`)
- **THEN** all 4 servos are attached with their respective pulse endpoints and frequencies

---

### Requirement: PinMapper hardware token resolution for 4 drivers
`PinMapper` SHALL define distinct non-GPIO hardware identifiers for all 4 driver slots:
- `DRIVER_A`: `0xE1`
- `DRIVER_B`: `0xE2`
- `DRIVER_C`: `0xE3`
- `DRIVER_D`: `0xE4`

`PinMapper::resolve()` SHALL resolve `"DRIVER_A"` through `"DRIVER_D"` to their respective constants, and `PinMapper::isDriver()` SHALL return `true` for all 4 driver names.

#### Scenario: Resolving DRIVER_C and DRIVER_D
- **WHEN** `PinMapper::resolve("DRIVER_C")` or `PinMapper::resolve("DRIVER_D")` is called
- **THEN** it returns `0xE3` and `0xE4` respectively

#### Scenario: Identifying driver tokens
- **WHEN** `PinMapper::isDriver("DRIVER_C")` or `PinMapper::isDriver("DRIVER_D")` is evaluated
- **THEN** it returns `true`
