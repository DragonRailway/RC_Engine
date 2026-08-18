# Proposal: Board Power & Charging Visual Indicators

## Why

To enhance user experience and provide clear, intuitive visual status feedback on ESP32-based RC vehicle controllers:
1. Users need a dedicated `"charging"` configuration section in the hardware config allowing assignment of a charging indicator output (via pin token or light alias) with configurable animation modes (`solid`, `blink`, `pulse`).
2. Users need a power indicator output in the `"power"` configuration section that provides **active visual feedback during button holds** (blinking rapidly for `button_hold_s` while holding the power button, then turning off when power-off occurs) and drives visual alerts during the 10-second disconnect warning phase.

## What Changes

1. **Configurable `"charging"` Section**:
   - Schema & Config struct: `hardware` (pin token or light alias) + `mode` (`"solid"`, `"blink"`, `"pulse"`).
   - Driven when `HardwareInit::isCharging()` is true.

2. **Configurable `"power"` Indicator & Button-Hold Feedback**:
   - Schema & Config struct: add `"hardware"` to `"power"` section.
   - When holding power button (`s_powerButtonHolding`), indicator blinks rapidly (200ms ON / 200ms OFF).
   - When `button_hold_s` threshold is reached, indicator turns OFF and `powerOff()` triggers.
   - When in 10-second disconnect warning phase, indicator blinks warning pattern alongside audio alert.

3. **Fallback & Alias Support**:
   - Hardware tokens support direct pin names (`L0`..`L8`, `S1`..`S4`) and light aliases (`head_light`, `tail_light`, `brake_light`, `cab_light`, `step_light`, `aux_light`).
   - If unassigned (`0xFF`), sensible fallbacks (hazards for power warning/hold, headlights/onboard LED for charging) ensure clear operation.

4. **Documentation & Validation**:
   - Update `GUIDE/HARDWARE_CONFIG.md` (sections 7 & 8).
   - Extend host VC driver unit test suite (Test 13).
   - Verify config validation and PlatformIO firmware build.

## Non-goals

- Hardware fuel gauge percentages via RGB NeoPixel strings (requires additional LED strip library).
