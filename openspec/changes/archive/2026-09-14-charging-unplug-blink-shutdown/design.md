## Context

ESP32-S3 boards running RC Engine latch hardware battery power (`POWER_ENABLE`) at boot. When USB power is connected without the power button being held, the firmware enters Minimal Charging Mode. Because `POWER_ENABLE` is latched ON, the battery continues to power the board even if the USB cable is disconnected.

Currently, `VehicleController::update()` detects USB disconnection and immediately calls `HardwareInit::powerOff()`. The board cuts `POWER_ENABLE` in 0 ms, providing no visual indication to the user that the vehicle recognized the unplug and safely turned off.

## Goals / Non-Goals

**Goals:**
- Provide immediate, deterministic visual feedback when USB power is disconnected in charging mode: 4 quick blinks (200ms period: 100ms ON / 100ms OFF, totaling 800ms).
- Keep `POWER_ENABLE` latched during the entire 4-blink sequence.
- Release `POWER_ENABLE` immediately upon completion of the 4th blink to power down the vehicle completely.
- Support the fallback cascade for the indicator pin (charging pin -> `power.indicatorPin` -> headlight -> turn signals).

**Non-Goals:**
- Blinking when powering off from RUN mode via 4-second hold (that already has its own rapid blink feedback during the hold).
- Non-blocking state machine during shutdown (since the board is terminating immediately, a clean synchronous farewell pulse avoids task scheduling races).

## Decisions

### Decision 1: Dedicated `HardwareInit::powerOffWithBlink` Method
- **Choice**: Add `static void powerOffWithBlink(int8_t indicatorPin, uint8_t blinks = 4, uint16_t onMs = 100, uint16_t offMs = 100)` to `HardwareInit`.
- **Rationale**: Keeps power sequencing encapsulated in `HardwareInit`. The routine drives the specified indicator pin high/low using `setLight(pin, duty)`, pauses with `delay()`, and then calls `powerOff()`.
- **Alternatives Considered**: Managing blink state asynchronously in `VehicleController::update()` with timers. Rejected because USB is disconnected, the system is actively dying, and an asynchronous timer introduces risk of premature task exit or delayed power-off.

### Decision 2: 200ms Blink Period (100ms ON / 100ms OFF)
- **Choice**: 4 cycles of 100ms ON and 100ms OFF (total sequence duration = 800ms).
- **Rationale**: 200ms per cycle is snappy yet distinct and clearly countable by the human eye. 800ms total battery consumption is negligible (< 0.01 mAh).

### Decision 3: Fallback Indicator Cascade
- **Choice**: In `VehicleController::update()`, resolve the blink target as:
  1. `s_hw->charging.pin` (if configured)
  2. `s_hw->power.indicatorPin` (if configured)
  3. `s_hw->lights.headLight.pin` (if configured)
  4. `turnLPin` / `turnRPin` (if configured)

## Risks / Trade-offs

- **[Risk] Watchdog timer triggers during the 800ms blocking sequence**:
  - **Mitigation**: The FreeRTOS task watchdog on ESP32 is typically 5000ms. An 800ms sequence with 100ms delays allows internal FreeRTOS yield/tick processing and is well within the watchdog threshold.
- **[Risk] User re-plugs USB while 4-blink sequence is running**:
  - **Mitigation**: Disconnecting USB is an intentional physical action. If plugged back in mid-sequence, the board still powers off cleanly; subsequent USB power re-triggers a clean boot cycle.
