## Context

On ESP32-S3 and classic ESP32, the MCPWM peripheral timer clock source is `SOC_MOD_CLK_PLL_F160M`. ESP-IDF's driver assumes this clock is fixed at 160 MHz and computes the hardware prescaler divider as `160,000,000 / resolution_hz`.
When `board_build.f_cpu = 80000000L` is configured, the clock feeding MCPWM drops to 80 MHz. The prescaler (160) causes the hardware timer to tick at 500 kHz (2 µs/tick) instead of 1 MHz (1 µs/tick). Commanded 1500 µs pulses double to 3000 µs at 25 Hz, freezing RC servos.
Migrating servos to LEDC was evaluated and rejected because LEDC channels are strictly reserved for vehicle lighting automation (`EasyLED`).

## Goals / Non-Goals

**Goals:**
- Make `ESP32_EasyKit` (`EasyServo` and `EasyMotor`) transparently agnostic to CPU clock frequency scaling on chips using PLL_F160M for MCPWM.
- Maintain exact 1 µs hardware timer ticks for servos across 80 MHz, 160 MHz, and 240 MHz CPU speeds.
- Maintain ultrasonic 20 kHz PWM for motors at 80 MHz without audible whine.
- Preserve all existing public APIs and internal easing/mapping mathematics.
- Verify hardware operation with `MIKRO_V2_SWEEP` and full `MIKRO_V2` firmware.

**Non-Goals:**
- Moving servo control to LEDC (reserved for lights).
- Supporting dynamic runtime CPU frequency scaling (DFS/DVFS) while a pulse is mid-cycle.

## Decisions

### Decision 1: Scale `timer_cfg.resolution_hz` by `(160 / actual_cpu_mhz)`
* **Rationale**: The ESP-IDF MCPWM driver calculates `prescale = idf_assumed_hz / requested_resolution_hz`. If we scale `resolution_hz = target_resolution * (160 / actual_mhz)`, the driver computes:
  $$\text{prescale} = \frac{160\,\text{MHz}}{\text{target\_resolution} \times \frac{160}{\text{actual\_mhz}}} = \frac{\text{actual\_mhz}}{\text{target\_resolution}}$$
  In hardware:
  $$\text{Actual Tick Rate} = \frac{\text{actual\_mhz}}{\text{prescale}} = \text{target\_resolution}$$
  This keeps actual hardware ticks at exactly 1 µs (for servo) and 10 MHz (for motor) without modifying any comparator or period register logic.
* **Alternative Considered**: Modifying compare values and period ticks. Rejected because it would distort timer wrap-around frequencies, break 16-bit register limits, and complicate easing math.

### Decision 2: Centralize compensation in `mcpwm_manager.h`
* **Rationale**: `mcpwm_manager.h` already manages MCPWM allocations and includes SOC headers. An inline function `EasyKit::getMcpwmTimerResolution(uint32_t desired_res_hz)` cleanly encapsulates SoC detection (`SOC_MCPWM_SUPPORT_XTAL` vs `SOC_MOD_CLK_PLL_F160M`) and clock calculation for both `EasyServo` and `EasyMotor`.
* **Alternative Considered**: Embedding raw calculations inside `EasyServo.cpp` and `EasyMotor.cpp` separately. Rejected to avoid code duplication and logic drift.

## Risks / Trade-offs

- **[Risk]** SoC without XTAL but with non-160 MHz base clock.
  - *Mitigation*: Guard with `#if defined(SOC_MCPWM_SUPPORT_XTAL)`. Chips supporting XTAL (C6, H2, P4) pass `desired_res_hz` directly. Chips on PLL_F160M (S3, classic ESP32) query `getCpuFrequencyMhz()`. If `cpu_mhz >= 160`, no compensation is applied.
- **[Risk]** PlatformIO dependency caching stale git version.
  - *Mitigation*: Apply and test changes in local `ESP32_EasyKit` sibling repo, and ensure PlatformIO picks up the local path or updated commit during verification.
