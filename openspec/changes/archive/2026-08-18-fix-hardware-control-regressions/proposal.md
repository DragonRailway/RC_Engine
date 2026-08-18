## Why

Motor, lighting, and turn signal toggle controls experienced regressions on the hardware:
1. `gas_pedal` and `brake_pedal` operate on a `[-100, +100]` spring-centered domain where negative values represent the idle/released pedal position. The control loop clamped negative values to 0, turning the entire lower half of pedal travel into a deadzone.
2. When disconnected for $>60\text{s}$, the power-saving auto-off pulled `POWER_ENABLE` (GPIO 15) `LOW`, cutting power to the H-bridge motor driver and LED MOSFETs. Reconnecting over BLE did not re-assert `POWER_ENABLE` `HIGH`, leaving the power rails at 0V.
3. Turn signal toggle buttons entered an infinite auto-cancel / re-trigger loop because `RK_ToggleButton` has no downstream sync to the app UI, causing subsequent BLE frames from the app to re-trigger edge detection repeatedly.
4. With 13 LED instances and only 8 hardware LEDC channels on the ESP32-S3, unpooled LED allocation exhausted hardware timer channels, causing lights on channels 8+ to fail attachment.

## What Changes

- **Motor & Pedal Control**:
  - Remap `gas_pedal` and `brake_pedal` inputs from `[-100, +100]` to `[0, 100]%` (`(val + 100) / 2`), restoring full progressive linear control.
  - Automatically re-latch `POWER::POWER_ENABLE` to `HIGH` whenever an active BLE connection is established or restored.
- **Lighting & LEDC Pooling**:
  - Pool / consolidate LED instances so total active LEDC channels on ESP32-S3 never exceed the hardware limit (8 channels).
  - Ensure all configured LEDs (`headLight`, `fullBeam`, `fogLamp`, `tailLight`, `brakeLight`, `turnLight`, `reversingLight`, `auxLight`) have active, working channel assignments and follow power latch state.
- **Turn Signal Toggle Logic**:
  - Implement app-suppression edge latching for `left_indicator` and `right_indicator`: when auto-cancelled by steering return or opposite steer in firmware, suppress re-triggering until the app user explicitly taps the button back to `0`.
  - Fix steering baseline and cancellation delta math to prevent premature cancellation when engaged during existing turns.

## Capabilities

### Modified Capabilities
- `vehicle-control-loop`: Update pedal range mapping (`[-100, 100]` to `[0, 100]%`), connection power-latch re-assertion, and drive motor dispatch.
- `advanced-lighting-automation`: Update manual turn signal toggle button latching, auto-cancellation re-trigger suppression, and mutual exclusion.
- `board-power-control`: Re-assert `POWER_ENABLE` `HIGH` on BLE connection event.

## Impact

- `common/VehicleController.h`: Throttle and brake pedal normalization, turn signal latch suppression, and power rail connect re-latching.
- `common/HardwareInit.h`: Consolidated LEDC channel management and power latch helpers.
- `test/host_vc/host_vc_driver.cpp`: Updated unit test assertions for pedal mapping and turn signal suppression.
