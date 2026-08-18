# Design: Board Power Management & 3-State Power Control

## D1: Configurable Power Parameters (in Seconds)

`HardwareConfig::Power` struct in `common/Config.h`:

```cpp
struct Power {
    float bootLatchS = 1.0f;          // Range: 0.0 - 30.0s
    float buttonHoldS = 4.0f;         // Range: 1.0 - 30.0s
    float disconnectTimeoutS = 60.0f; // Range: 0.0 - 3600.0s (0 = disabled)
    float warningWindowS = 10.0f;     // Range: 1.0 - 60.0s
    float cutoffDelayS = 1.5f;        // Range: 0.0 - 60.0s
} power;
```

`ConfigParser.h` converts JSON numbers (in seconds) to internal timers.
Hardware pins (`POWER_ENABLE`, `POWER_BUTTON`, `CHARGE_SENS`) check against `0xFF` so boards lacking power control or charge sensing gracefully skip unneeded routines.

---

## D2: 3-State Power Management (`OFF`, `ON`, `CHARGING`)

```
   State Flow:
   
   ┌──────────┐  POWER_ENABLE HIGH   ┌──────────┐
   │   OFF    │ ───────────────────► │    ON    │
   └──────────┘                      └────┬─────┘
                                          │
                   CHARGE_SENS == HIGH    │   CHARGE_SENS == LOW
                 ┌────────────────────────┴────────────────────────┐
                 ▼                                                 ▼
          ┌──────────────┐                                  ┌──────────────┐
          │   CHARGING   │                                  │      ON      │
          │              │                                  │              │
          │ - Motors = 0 │                                  │ - Normal VC  │
          │ - Auto-Off   │                                  │ - Disconnect │
          │   suspended  │                                  │   auto-off   │
          └──────────────┘                                  └──────────────┘
```

When `CHARGE_SENS != 0xFF` and `digitalRead(POWER::CHARGE_SENS) == HIGH`:
- Board enters `STATE_CHARGING`.
- `HardwareInit::setAllMotors(0)` keeps drive outputs safe.
- Disconnected auto power-off timer is suspended.

---

## D3: Disconnect Auto Power-Off with 10s Warning & Button Reset

In `VehicleController::update()`:

1. If `RadioKit.isConnected()`:
   - `s_disconnectStart = 0`
   - `s_inWarningPhase = false`

2. If `!RadioKit.isConnected()` and `disconnectTimeoutS > 0`:
   - Track `now - s_disconnectStart`.
   - If `remaining <= warningWindowS`:
     - Set `s_inWarningPhase = true`.
     - Drive hazard blink / light warning and periodic warning alert sound.
   - If `now - s_disconnectStart >= disconnectTimeoutS * 1000`:
     - Execute `HardwareInit::powerOff()`.

3. Power Button Click Detection during Disconnect:
   - In `HardwareInit::updatePowerButton()`, detect a short press/click (`HIGH` to `LOW` release under `buttonHoldS`).
   - If short press detected while disconnected:
     - Reset `s_disconnectStart = millis()`.
     - Clear `s_inWarningPhase = false`.
     - Cancels warning phase and grants a full fresh `disconnectTimeoutS` window.
