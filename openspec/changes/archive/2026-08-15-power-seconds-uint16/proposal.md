# Proposal: Use uint16_t for Power Time Parameters in Seconds

## Why

Currently, board power management timing parameters (`boot_latch_s`, `button_hold_s`, `disconnect_timeout_s`, `warning_window_s`, `cutoff_delay_s`) are represented as floating-point numbers (`float`). Since these timing durations are specified in integer seconds, using integer types (`uint16_t` in C++ / `"type": "integer"` in schema) eliminates floating-point arithmetic overhead in embedded time comparisons, prevents precision loss, and simplifies configuration syntax.

## What Changes

1. **Schema & Config Struct Updates**:
   - Update `configs/schemas/hardware_config.schema.json`: change power timing parameters to `"type": "integer"`.
   - Update `HardwareConfig::Power` in `common/Config.h`: change `float` to `uint16_t` for `bootLatchS`, `buttonHoldS`, `disconnectTimeoutS`, `warningWindowS`, `cutoffDelayS`.
   - Update `common/ConfigParser.h`: parse integer fallbacks and constrain `uint16_t` ranges.

2. **Firmware & API Updates**:
   - Update `HardwareInit.h`: adjust `latchPower()`, `update()`, and `updatePowerButton()` parameters to `uint16_t`.
   - Update `VehicleController.h`: use `uint16_t` for disconnect timeout and warning window calculations.
   - Update shipped hardware configs (`hardware-*.json`): update float values (e.g. `1.0`, `4.0`, `60.0`, `10.0`, `1.5`) to integer values (`1`, `4`, `60`, `10`, `2`).

3. **Documentation & Tests**:
   - Update `GUIDE/HARDWARE_CONFIG.md`: document `uint16_t` / integer type for `power` section parameters.
   - Update host VC driver test suite (`test/host_vc/host_vc_driver.cpp`).
   - Run validation, host tests, and PlatformIO builds (`TRACKLINK_V3` and `MIKRO_V2`).

## Non-goals

- Changing timing values to milliseconds (timing parameters remain in integer seconds).
