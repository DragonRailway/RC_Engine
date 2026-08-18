# Proposal: Board Power Management & 3-State Power Control

## Why

Currently, hardware power latching and battery safety timing values are hardcoded in milliseconds. To maximize usability, safety, and pack longevity across diverse RC vehicle boards:
1. The board needs a **disconnect auto power-off** feature (default 60 seconds) so that an unattended or unlinked vehicle automatically powers off to prevent battery depletion.
2. During the final **10 seconds** of disconnection (`warning_window_s`), active hazard warning blinks and audio warning alerts should inform the user, and clicking the physical power button should **reset** the power-off timer back to 0.
3. The board supports **three operational states**: `OFF`, `ON`, and `CHARGING` (detected via the `CHARGE_SENS` pin). Motor outputs and disconnect auto-off must be safely suspended while charging.
4. Boards differ in hardware capability — power control (`POWER_ENABLE`, `POWER_BUTTON`) and charging sense (`CHARGE_SENS`) must be optional (`0xFF` pin guard).
5. All time parameters in the configuration schema must be expressed in **seconds** (with expanded, flexible ranges) for clear user configuration.

## What Changes

1. **Configurable `"power"` Block (in seconds)**:
   - Add `"power"` section to `configs/schemas/hardware_config.schema.json`:
     - `boot_latch_s`: number, default `1.0`, range `0.0`–`30.0`
     - `button_hold_s`: number, default `4.0`, range `1.0`–`30.0`
     - `disconnect_timeout_s`: number, default `60.0`, range `0.0`–`3600.0` (0 = disabled)
     - `warning_window_s`: number, default `10.0`, range `1.0`–`60.0`
     - `cutoff_delay_s`: number, default `1.5`, range `0.0`–`60.0`
   - Update `HardwareConfig::Power` struct in `common/Config.h` and parser in `common/ConfigParser.h`.

2. **3-State Board Power Engine (`OFF`, `ON`, `CHARGING`)**:
   - `OFF`: Hardware rail unlatched / power disabled (`POWER_ENABLE` LOW).
   - `ON`: Normal vehicle operation and control loop pump.
   - `CHARGING`: Triggered when `CHARGE_SENS` pin is active. Suspends motor drives and disconnect auto power-off.

3. **Disconnect Auto Power-Off with 10s Warning & Button Reset**:
   - Monitors `RadioKit.isConnected()`.
   - When disconnected for `disconnect_timeout_s - warning_window_s`, triggers hazard blink & audio alert.
   - A single click on `POWER_BUTTON` while disconnected or during warning resets the timer.
   - Reconnecting the app resets the timer.
   - If timeout elapses without button click or app reconnection, `HardwareInit::powerOff()` executes.

4. **Board Hardware Capability Guards**:
   - Graceful skip of power latching, button monitoring, or charge sensing when pins are `0xFF`.

5. **Documentation & Tests**:
   - Update `GUIDE/HARDWARE_CONFIG.md` (sections 4.7, 7, and board pin references).
   - Extend host VC driver test suite (`test/host_vc/host_vc_driver.cpp`).

## Non-goals (explicitly deferred)

- Multi-cell independent balance charge sensing (handled by onboard hardware charger IC).
- Remote software wake-up over BLE while in `OFF` state (requires persistent deep-sleep RTC hardware).
