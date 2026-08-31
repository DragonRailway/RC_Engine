## Context

Currently, `applyLightsWithAutomation()` controls turn and hazard signals by calling generic `HardwareInit::setLightBlink()` on `leftPin` and `rightPin` separately. When Left is already blinking and Hazard is engaged, Left is ignored while Right starts a new blink timer, resulting in a desynchronized, alternating blink.

## Goals / Non-Goals

**Goals:**
- **Synchronized Hazard Flashing**: Ensure Left and Right LEDs flash in 100% synchronous phase (0 deg offset) during Hazard mode.
- **Clean State Transitions**: Seamless transitions between OFF, Left-only, Right-only, and Hazard without timer desync.
- **Audio Click Phase Alignment**: Trigger the flasher relay audio click at the start of each synchronous flash cycle.

**Non-Goals:**
- Changing individual LED PWM driver hardware or EasyLED library internals.

## Decisions

### 1. First-Class `HardwareInit::setTurnSignals()` Interface
- **Decision**: Introduce `HardwareInit::setTurnSignals(bool left, bool right, bool hazard, uint16_t onMs, uint16_t offMs, uint8_t brightness)` backed by an explicit state machine:
  ```cpp
  enum class TurnMode { OFF, LEFT, RIGHT, HAZARD };
  ```
- **Rationale**: Replaces uncoordinated pin-level calls with a central coordinator that detects state transitions and resets both timers in unison.

### 2. Simultaneous Timer Reset on Hazard Entry
- **Decision**: When entering `TurnMode::HAZARD` (or when both left and right are active), stop both `turnLLed` and `turnRLed` and restart them in the same millisecond.
- **Rationale**: Guarantees zero phase drift regardless of which turn signal was previously active.

### 3. Audio Click Synchronization
- **Decision**: In `VehicleController.cpp`, reset `s_lastIndicatorClick` upon mode transitions so the audio flasher click triggers simultaneously with the lights turning ON.

## Risks / Trade-offs

- **[Risk]** Rapid app toggle between left and hazard creating brief LED blips.
  → **Mitigation**: State machine only restarts timers when `TurnMode` genuinely changes.
