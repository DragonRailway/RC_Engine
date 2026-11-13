## Why

When connected via USB without a RadioKit peer connected, the TRACKLINK_V2 board (and other ESP32-S3 boards) bootloops every few minutes. The board triggers a disconnect auto-power-off, but because USB VBUS keeps the 3.3V rail energized, the current `esp_deep_sleep(0)` call sets a 0 microsecond RTC timer that immediately wakes up and resets the board. Furthermore, the board lacks a true minimal charging mode: it runs the full vehicle stack (BLE modem advertising, audio synthesis, high-frequency control tasks) even while docked or plugged in to charge, wasting energy, generating internal heat inside closed vehicle bodies, and misclassifying full-battery docked states as abandoned vehicles.

## What Changes

- **Minimal Charging Mode**: Introduce a dedicated minimal charging mode when USB/charging is detected at boot or while running:
  - **Modem OFF**: Bluetooth / RadioKit BLE advertising is completely disabled in charging mode.
  - **Audio & Actuators OFF**: Audio DAC/I2S DMA is stopped and muted; servos and motor drivers remain parked.
  - **Disconnect Auto-Off Inhibit**: Disconnect timeout is suspended while plugged into USB or charging.
- **Heartbeat & End-of-Charge LED Indicators**:
  - **Active Charging**: Configured charging indicator LED blinks a double-pulse heartbeat pattern (120ms ON, 120ms OFF, 120ms ON, 640ms OFF) at 100% of configured maximum brightness.
  - **End of Charging**: When charging is complete (battery full / `CHARGE_SENS` inactive while still connected to USB), the indicator transitions to solid ON at 20% of configured maximum brightness.
- **Power Button Exit ("Switch On")**:
  - Pressing the physical power button while in charging mode exits charging mode, starts the RadioKit BLE modem, launches audio, and transitions into the active operational (`ON`) vehicle state.
- **USB Unplug Clean Power-Off**:
  - Unplugging USB while in charging mode cleanly powers off the board (`POWER_ENABLE = LOW`).
- **Power-Off Bugfix**:
  - Replace the flawed `esp_deep_sleep(0)` in `HardwareInit::powerOff()` which arms a 0µs timer and triggers infinite bootloops when USB power is present.

## Capabilities

### New Capabilities
<!-- None -->

### Modified Capabilities
- `board-power-management`: Add minimal charging mode with modem and audio disabled, power button switch-on transition, USB presence tracking to inhibit disconnect timeout, and fix `esp_deep_sleep(0)` bootlooping.
- `board-power-indicators`: Update charging indicator requirements to specify heartbeat double-pulse during active charging and solid 20% maximum brightness upon charge completion.

## Impact

- `common/VehicleController.cpp` & `common/VehicleController.h`: Update power and charging state machine, disconnect timeout inhibit logic, and indicator animation routines.
- `common/HardwareInit.cpp` & `common/HardwareInit.h`: USB power detection, `esp_deep_sleep(0)` fix in `powerOff()`, and power button click handling during charging mode.
- `src/main.cpp`: Defer/condition RadioKit BLE initialization and audio task startup when booting into USB charging mode until power button switch-on is triggered.
- `configs/schemas/hardware_config.schema.json` & hardware configs: Retain compatibility with existing `"charging"` block (`hardware`, `mode`).
