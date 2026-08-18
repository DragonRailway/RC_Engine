## Context

Hardware testing via RadioKit Remote API and USB-JTAG on the `MIKRO_V2` board surfaced control pipeline discrepancies:
- `gas_pedal` and `brake_pedal` are defined in `src/RADIOKIT.h` as `RK_GasPedal` with `RK_SPRING_MIN` (values `-100` to `+100`, resting at `-100`). `VehicleController` previously clamped `throttleInput > 0`, discarding all input when `gas_pedal <= 0` (0 to 50% physical pedal travel).
- Disconnect auto-power-off calls `HardwareInit::powerOff()` (`POWER_ENABLE = LOW`). Upon BLE connection re-establishment, `POWER_ENABLE` was not re-asserted, preventing 3.3V/VBAT power delivery to the DRV8833 H-bridge motor driver and LED MOSFETs.
- `RadioKit_Button` has no output synchronization channel (`outputSize() = 0`). When firmware auto-cancelled `left_indicator` or `right_indicator` on steering return, the app continued transmitting `1`, causing the firmware to perceive each subsequent frame as a new press edge and enter an infinite oscillation loop.
- ESP32-S3 has 8 hardware LEDC timer channels. `HardwareInit` attempted to initialize up to 13 separate `EasyLED` objects without channel pooling, causing channels beyond 8 to fail attachment.

## Goals / Non-Goals

**Goals:**
- Provide 0..100% linear progressive throttle and brake response across full physical pedal travel.
- Ensure power rail (`POWER::POWER_ENABLE`) is guaranteed `HIGH` whenever BLE is connected.
- Eliminate turn signal re-trigger loops via edge-latch state machine with app-suppression until the app resets to `0`.
- Pool LEDC channels so total active hardware channels never exceed 8, ensuring all lights attach successfully.

**Non-Goals:**
- Modifying the upstream RadioKit BLE wire protocol or widget definitions.
- Changing vehicle sound profiles or audio sample rates.

## Decisions

### 1. Pedal Normalization Formula
- **Decision**: Normalize `gas_pedal` and `brake_pedal` using `(rk.value + 100) / 2` (converting `[-100, +100]` to `[0, 100]%`).
- **Rationale**: `RK_GasPedal` operates with `RK_SPRING_MIN` where `-100` is the released baseline and `+100` is 100% full depression.
- **Alternatives considered**: Changing `min` in `src/RADIOKIT.h` to `0` — rejected because `RK_Slider` / `RK_GasPedal` in `RadioKitLib` natively uses signed `[-100, +100]` bounds.

### 2. Turn Signal App-Suppression State Machine
- **Decision**: Introduce `s_leftIndSuppressed` and `s_rightIndSuppressed` flags. When auto-cancelled by steering return or opposite steer, set the suppressed flag to `true`. While suppressed, incoming `state == true` frames are ignored. The suppression flag clears when the app sends `state == false`.
- **Rationale**: Prevents repeated BLE frames from re-arming the indicator while the app UI toggle remains in the ON position.

### 3. Re-latching Power on Connection
- **Decision**: In `VehicleController::update()`, whenever `RadioKit.isConnected()` is true, check `if (!HardwareInit::isPowerLatched()) HardwareInit::latchPower();` (or ensure `POWER::POWER_ENABLE` is driven `HIGH`).
- **Rationale**: Guarantees that reconnection after a power-down or timeout restores power to the motor drivers and LED rails without requiring a board reboot.

### 4. LEDC Channel Pooling
- **Decision**: Ensure that lights sharing pins (e.g. `reversingLight` sharing `brakeLight`) do not allocate duplicate channels, and attach only configured pins. For ESP32-S3 with 8 channels, allocate channels dynamically or pool simple on/off lights where appropriate.

## Risks / Trade-offs

- [Risk] If an indicator is auto-cancelled by steering return, the phone app UI toggle button may still visually look "toggled ON" until tapped.
  → Mitigation: The driver will tap it once to turn it OFF (clearing suppression) and tap again to re-engage, exactly matching typical non-bidirectional RC transmitter switches.
