## Why

When an ESP32-S3 RC vehicle controller is connected to USB power without being switched ON, it operates in Minimal Charging Mode with `POWER_ENABLE` latched ON to keep the battery rail connected. Currently, when the USB cable is unplugged, the board abruptly executes `HardwareInit::powerOff()` in 0 ms, shutting down without any user feedback.

Providing an immediate 4-blink acknowledgment sequence (200ms cycle: 100ms ON / 100ms OFF) on the charging indicator before cutting `POWER_ENABLE` provides clear, responsive visual feedback that the vehicle acknowledged the unplug and safely powered off.

## What Changes

- **Unplug Feedback Animation**:
  - When USB is disconnected while in `CHARGING_ACTIVE` or `CHARGING_COMPLETE` (and was not switched ON), the board executes a 4-blink sequence (100ms ON, 100ms OFF, totaling 800ms).
  - Uses the indicator pin cascade: configured charging pin -> `power.indicatorPin` -> headlight pin -> turn signals.
- **Synchronous Safe Latch Release**:
  - The battery power latch `POWER_ENABLE` remains firmly held for the duration of the 4 blinks.
  - Immediately following the 4th blink, `POWER_ENABLE` is driven OFF to completely power down the hardware.

## Capabilities

### Modified Capabilities
- `board-power-management`: Add requirement for 4-blink (200ms cycle) charging indicator feedback before `POWER_ENABLE` release upon USB disconnection in charging mode.

## Impact

- Affected Code:
  - `common/HardwareInit.h`, `common/HardwareInit.cpp`: Add `powerOffWithBlink(int8_t indicatorPin, uint8_t blinks = 4, uint16_t onMs = 100, uint16_t offMs = 100)`.
  - `common/VehicleController.cpp`: Call `HardwareInit::powerOffWithBlink()` upon USB disconnect in charging mode.
  - `test/host_vc/host_vc_driver.cpp`: Verify the unplug blink and shutdown sequence in host simulation.
