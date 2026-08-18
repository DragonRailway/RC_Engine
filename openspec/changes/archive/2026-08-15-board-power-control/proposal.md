# Proposal: Board Power Control & Two-Tier Battery Protection

## Why

Currently, the ESP32 RC vehicle controller hardware boards (`TRACKLINK_V3` and `MIKRO_V2`) define `POWER::POWER_ENABLE` (GPIO pin to latch board VCC ON) and `POWER::POWER_BUTTON` (GPIO input pin connected to the physical power button).

However:
1. Releasing the physical power button immediately after pressing it turns off the board if power is not latched by the firmware.
2. Short accidental taps (< 1000ms) on the button should not latch board power on.
3. At runtime, long-pressing the power button (>= 4000ms) should gracefully stop all outputs and shut down board power by setting `POWER_ENABLE` to LOW.
4. Battery protection currently halts motor output on cutoff but leaves hardware power ON. Adding a **two-tier battery protection** (Warning Voltage at 3.5V/cell vs Cutoff Voltage at 3.3V/cell) enables warning alerts prior to a complete hardware power-off cutoff.

## What Changes

1. **Boot Power Latching Filter (1000ms)**:
   - At early boot in `setup()`, the firmware monitors `POWER_BUTTON` (Active HIGH).
   - If `POWER_BUTTON` remains HIGH for **>= 1000ms**, `POWER_ENABLE` is set `HIGH` (latched).
   - If released before 1000ms, power is not latched, preventing accidental brief taps from keeping the board powered on.

2. **Runtime Long-Press Power-Off (4000ms)**:
   - In the main control loop, monitor `POWER_BUTTON` (Active HIGH).
   - If held continuously for **>= 4000ms**, execute a clean shutdown (`stopAll()`, audio stop) and drive `POWER_ENABLE` `LOW`.
   - Short presses (< 4000ms) are ignored.

3. **Two-Tier Battery Protection**:
   - **Warning Voltage** (`battery.warning_voltage`, default 3.5V per cell):
     - Triggers low-voltage warning sound / telemetry warning / alert state while motor drive remains enabled.
   - **Cutoff Voltage** (`battery.cutoff_voltage`, default 3.3V per cell):
     - After 1500ms below cutoff, stops all motor outputs and drives `POWER_ENABLE` `LOW` to hardware power-off the board.

4. **Schema & Config Parser Updates**:
   - Update `configs/schemas/hardware_config.schema.json` to accept optional `warning_voltage` (default 3.5V/cell) and set default `cutoff_voltage` to 3.3V/cell.
   - Update shipped hardware configs (`hardware-*.json`).

## Non-goals (explicitly deferred)

- Complex multi-click gestures (e.g. double-click, triple-click). Short presses (< 4000ms) are ignored in this phase.
- Software power-off timer when idle (auto-sleep / auto-off on BLE disconnect deferred).
