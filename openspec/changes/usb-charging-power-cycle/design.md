## Context

On ESP32-S3 boards (MIKRO_V2, TRACKLINK_V2, TRACKLINK_V3), power can be delivered either by the battery (controlled by `POWER_ENABLE` latching) or via 5V USB VBUS.

When USB power is connected, the 3.3V LDO remains powered regardless of `POWER_ENABLE`. In the current implementation, triggering power-off (4s button hold or idle timeout) while on USB calls `HardwareInit::powerOff()`, which unlatches `POWER_ENABLE` and puts the ESP32 into `esp_light_sleep_start()`. This causes:
1. Battery charging stops or loses latch stability.
2. The internal USB-Serial-JTAG unit clock-gates, breaking flashing and serial monitoring until hard reset.
3. No visual charging indicator is shown.
4. The board cannot be turned "OFF" because USB still supplies power, yet it is trapped in light sleep rather than functioning as a charger.

Instead, the board lifecycle must cleanly separate battery-powered shutdown from USB-powered charging fallback.

## Goals / Non-Goals

**Goals:**
- Booting with USB connected without power button hold starts directly in Minimal Charging Mode.
- Booting with power button hold starts directly in RUN mode.
- Switching OFF (4s button hold or disconnect timeout) while USB is connected transitions cleanly into Minimal Charging Mode:
  - RadioKit BLE modem stops and disconnects any client.
  - Audio synthesis stops and mutates to OFF state.
  - Motors stop, servos sleep.
  - `POWER_ENABLE` remains latched ON to allow continuous battery charging.
  - Charging indicator animation runs.
  - USB-Serial-JTAG remains fully responsive for serial logs and firmware flashing (`pio run -t upload`).
- Clicking power button in Charging Mode returns to RUN mode.
- Unplugging USB in Charging Mode executes the 4-blink farewell and cuts `POWER_ENABLE` to enter zero-power OFF mode.

**Non-Goals:**
- Changing power-off behavior when running purely on battery (which already cuts `POWER_ENABLE` directly in 0ms).

## Decisions

### Decision 1: Symmetrical `stopOperationalStackToCharging()` in `src/main.cpp`
- **Choice**: Provide `stopOperationalStackToCharging()` in `src/main.cpp` that de-initializes BLE, stops audio, halts motors, sets `s_operationalMode = false`, sets `HardwareInit::setUsbBoot(true)`, and keeps `POWER_ENABLE` latched.
- **Rationale**: `startOperationalStack()` starts BLE and audio when transitioning from Charging Mode to RUN mode. `stopOperationalStackToCharging()` performs the exact inverse teardown when transitioning back to Charging Mode.
- **Alternatives Considered**: Calling `esp_restart()` into USB boot mode. Rejected because rebooting incurs LittleFS mount delays, re-parsing configs, and loses live debug state.

### Decision 2: Power-Off Request Dispatching
- **Choice**: When a 4-second button hold is detected in `HardwareInit::updatePowerButton` or a disconnect timeout occurs in `VehicleController`:
  - Check `HardwareInit::isUsbPowered() || HardwareInit::isCharging()`.
  - If USB power is active: signal/trigger transition to Charging Mode via `stopOperationalStackToCharging()`.
  - If USB power is NOT active: execute `HardwareInit::powerOff()` (or `powerOffWithBlink()`).
- **Rationale**: Keeps USB-powered boards fully operational as smart battery chargers and keeps serial JTAG communication alive at all times.

## Risks / Trade-offs

- **[Risk] User connects USB while vehicle is running in RUN mode**:
  - **Handling**: Board remains in RUN mode and simply charges the battery concurrently. Only explicit power-off (4s hold or timeout) transitions it back to Charging Mode.
- **[Risk] BLE reconnect attempts while shutting down to Charging Mode**:
  - **Handling**: Disconnecting all peer clients and calling `pAdv->stop()` ensures BLE central devices register immediate disconnection.
