# PWM EasyKit Integration

## Purpose

Defines the vendored ESP32_EasyKit PWM layer as the sole driver for all PWM outputs (motors via MCPWM, servos/ESCs via MCPWM PPM, lights via LEDC), replacing raw ledc calls and the legacy ESP32_PWM_Fusion dependency, while keeping the HardwareInit public API stable and supporting runtime hot-reload.

## Requirements

### Requirement: EasyKit Dependency Naming
The project SHALL reference the PWM library dependency as `ESP32_EasyKit`, not `ESP32_PWM_Fusion`, in `platformio.ini`, pointing at the vendored library at `lib/ESP32_EasyKit` (no machine-local symlink).

#### Scenario: Build with vendored dependency
- **WHEN** running `pio run -e TRACKLINK_V3` on a machine without the author's `~/Arduino/libraries` checkout
- **THEN** the build resolves the vendored `lib/ESP32_EasyKit` library and compiles successfully

#### Scenario: No stale Fusion references
- **WHEN** searching the repo for `ESP32_PWM_Fusion` in build, config, and reference files
- **THEN** no references remain and the obsolete `references/ESP32_PWM_Fusion` copy is deleted

### Requirement: EasyKit Hardware Abstraction
`common/HardwareInit.h` SHALL use EasyKit classes for all PWM outputs — `EasyMotor` for the drive motor (DRIVER_2PWM and DRIVER_1PWM_1DIR), `EasyServo` for the steering servo and ESC (PPM pulse output), and `EasyLED` for light channels — instead of raw `ledcAttach`/`ledcWrite` calls.

#### Scenario: H-Bridge A dual-PWM motor
- **WHEN** a config specifies `HBRIDGE_A` with dual PWM pins and an enable pin
- **THEN** an `EasyMotor` in DRIVER_2PWM mode drives both PWM pins, asserts the enable pin, and respects the configured min/max duty window

#### Scenario: H-Bridge B DIR+PWM motor
- **WHEN** a config specifies `HBRIDGE_B` with a direction pin and a PWM pin
- **THEN** an `EasyMotor` in DRIVER_1PWM_1DIR mode drives the PWM pin and toggles the direction pin for forward/reverse

#### Scenario: ESC output
- **WHEN** the drive motor type is `ESC`
- **THEN** an `EasyServo` instance at 50 Hz generates a PPM pulse between 1000 µs and 2000 µs proportional to throttle

#### Scenario: Light channels
- **WHEN** light pins are configured
- **THEN** each light is driven by an `EasyLED` instance at 5 kHz / 10-bit with brightness set as a percentage

### Requirement: Stable HardwareInit Public API
The `HardwareInit::setMotor(int16_t)`, `setServo(int16_t)`, and `setLight(pin, pct)` signatures SHALL remain unchanged so that `VehicleController`, `src/main.cpp`, and RadioKit control code do not require modification.

#### Scenario: Vehicle controller calls unchanged
- **WHEN** `VehicleController` calls `HardwareInit::setMotor`, `setServo`, and `setLight`
- **THEN** the calls compile and behave identically to the pre-change API

### Requirement: MCPWM/LEDC Channel Budget
The firmware SHALL stay within the ESP32-S3's 8 LEDC channels and SHALL use the MCPWM peripheral for motor and servo outputs so that a fully-loaded config (2 motor + 1 servo + 6 lights) fits within hardware limits.

#### Scenario: Fully-loaded configuration
- **WHEN** motor, steering servo, and all six light channels are configured
- **THEN** LEDC channel usage does not exceed 8 channels and MCPWM operator usage does not exceed 12 operators

### Requirement: EasyServo Pulse Timing Correctness
`EasyServo` SHALL generate servo pulse widths in microseconds accurately: a `writeMicroseconds(1500)` call SHALL produce a 1500 µs high pulse at the configured refresh rate.

#### Scenario: Microsecond pulse accuracy
- **WHEN** a servo is attached at 50 Hz and commanded to 1500 µs
- **THEN** the MCPWM output produces a 1500 µs pulse every 20 ms

#### Scenario: Frequency reconfiguration
- **WHEN** `setFrequency()` is called on an attached `EasyServo`
- **THEN** the underlying MCPWM timer period is reconfigured to match the new frequency

### Requirement: Hot-Reload Teardown
`HardwareInit::stopAll()` SHALL fully release EasyKit hardware (MCPWM timers/operators and LEDC channels) so a subsequent `init()` with a new configuration can re-attach without resource exhaustion.

#### Scenario: Config hot-reload
- **WHEN** a new hardware config is applied at runtime via `hotReload()`
- **THEN** previous EasyKit objects are `end()`ed, channels are freed, and the new configuration attaches successfully

### Requirement: References Documentation
The `references/` directory SHALL document which PWM libraries build on the project's Arduino 3.x / ESP-IDF 5.x toolchain and which use the removed legacy MCPWM API.

#### Scenario: Legacy libraries marked
- **WHEN** a developer reads `references/README.md`
- **THEN** they can determine that `ESP32Servo`, `ESP32MCServo`, and `ESP32_MCPWM` use the legacy `driver/mcpwm.h` API and require migration to build on this toolchain
