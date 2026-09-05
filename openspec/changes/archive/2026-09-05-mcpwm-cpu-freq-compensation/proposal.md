## Why

When the ESP32-S3 CPU frequency was reduced to 80 MHz (`board_build.f_cpu = 80000000L`) for battery power savings, the steering servo stopped responding.

Root-cause analysis revealed that the ESP-IDF v5 MCPWM driver on ESP32-S3 uses `SOC_MOD_CLK_PLL_F160M` as its timer clock source and calculates the hardware timer prescaler assuming a constant 160 MHz clock. When the CPU runs at 80 MHz, the actual clock feeding the MCPWM peripheral is scaled down to 80 MHz. The driver's uncompensated prescaler calculation cuts the hardware timer frequency in half (500 kHz instead of 1 MHz), resulting in a doubled pulse width (1500 µs commanded becomes 3000 µs actual) and halved PWM frequency (50 Hz becomes 25 Hz). Servos reject 3000 µs pulses and fail to actuate.

To preserve 80 MHz CPU operation for low-power battery life while keeping LEDC channels dedicated to vehicle lighting, MCPWM must dynamically compensate its timer configuration across CPU frequencies.

## What Changes

- **Compensate MCPWM Timer Resolution for CPU Clock**: Introduce a dynamic resolution compensation helper (`getMcpwmTimerResolution`) in `ESP32_EasyKit` that scales `timer_cfg.resolution_hz` by `(160 MHz / actual_src_clk)` on chips lacking hardware XTAL support for MCPWM (such as ESP32-S3 and ESP32).
- **Update EasyServo and EasyMotor MCPWM Initializers**: Apply the compensated resolution to `EasyServo::attach()` and `EasyMotor::begin()`, ensuring that timer ticks remain exactly 1 µs (1 MHz) for servo PPM and 10 MHz for ultrasonic motor PWM at both 80 MHz and 160/240 MHz CPU frequencies.
- **Update RC_brain Dependency Resolution**: Ensure `RC_brain` builds against the updated `ESP32_EasyKit` library with the clock compensation in place.
- **Hardware Sweep Verification**: Validate the fix using the dedicated `[env:MIKRO_V2_SWEEP]` target and full `[env:MIKRO_V2]` vehicle firmware running on physical hardware (`/dev/ttyACM0`).

## Capabilities

### New Capabilities
<!-- None -->

### Modified Capabilities
- `pwm-easykit-integration`: Add requirement for CPU clock frequency independence in MCPWM timing, ensuring exact 50 Hz / 1000–2000 µs PPM servo output and 20 kHz ultrasonic motor PWM when running at 80 MHz CPU frequency.

## Impact

- **Libraries Affected**:
  - `ESP32_EasyKit`: `src/common/mcpwm_manager.h`, `src/EasyServo.cpp`, `src/EasyMotor.cpp`.
- **Applications Affected**:
  - `RC_brain`: `platformio.ini`, `test/servo_sweep/servo_sweep_main.cpp`, `common/HardwareInit.h`.
- **Hardware Verified**: MIKRO_V2 (ESP32-S3) on `/dev/ttyACM0` at 80 MHz CPU frequency.
- **APIs & Compatibility**: No breaking changes to public APIs of `EasyServo`, `EasyMotor`, or `HardwareInit`. All angle-to-microsecond mappings, easing math, and comparator values remain unchanged.
