## Why

When an ESP32-S3 RC vehicle controller is connected to USB power, physical power to the MCU cannot be severed because 5V VBUS supplies the board. Currently, attempting to switch OFF while USB is connected puts the MCU into dormant light sleep, cuts battery power, terminates USB-Serial-JTAG debugging/flashing, and leaves the board unresponsive.

Instead, the board must operate in a deterministic 3-state power lifecycle: booting via USB starts directly in Minimal Charging Mode; switching ON with the power button enters RUN mode; and switching OFF while USB remains plugged in returns the board to Charging Mode with `POWER_ENABLE` latched to keep charging the battery, while keeping USB-JTAG responsive and indicator animations active.

## What Changes

- **Direct Boot to Charging Mode on USB Connection**:
  - Plugging in USB (or booting with USB connected without power button hold) powers up the board directly into Minimal Charging Mode (RadioKit BLE and audio synthesis remain stopped, `POWER_ENABLE` is latched ON for battery charging).
- **Run Mode Entry Exclusively via Power Button**:
  - The vehicle controller only enters full RUN mode when powered ON via power button hold at boot or clicked/pressed while in Charging Mode.
- **Graceful Shutdown to Charging Mode when USB is Plugged In**:
  - When the user executes power-off (4s button hold or disconnect idle timeout) while USB is connected, the board shuts down the operational stack (stops BLE advertising/connections, stops audio output, halts motors, sleeps servos) and transitions directly to Minimal Charging Mode.
  - `POWER_ENABLE` remains latched ON so the battery pack continues charging.
  - USB Serial/JTAG remains awake, healthy, and immediately flashable.
- **Complete Power-Off on Battery**:
  - If USB is disconnected when switching OFF, the board releases `POWER_ENABLE` to cut power completely.

## Capabilities

### Modified Capabilities
- `board-power-management`: Update operational lifecycle requirements: booting via USB enters Minimal Charging Mode directly; switching OFF while USB is connected transitions into Minimal Charging Mode instead of entering light sleep or cutting power.

## Impact

- `src/main.cpp`: Add `stopOperationalStackToCharging()` and update the power-off handling to gracefully cycle back to Minimal Charging Mode when USB is plugged in.
- `common/HardwareInit.h`, `common/HardwareInit.cpp`: Separate low-level hardware unlatch from the USB-connected charging state transition.
- `common/VehicleController.cpp`: Ensure timeout/cutoff power-off branches respect USB connection state.
- `test/host_vc/host_vc_driver.cpp`: Add unit tests for USB power-off transition into Charging Mode and subsequent wake back into Run Mode.
