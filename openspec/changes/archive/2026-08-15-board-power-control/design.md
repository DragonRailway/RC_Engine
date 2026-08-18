# Design: Board Power Control & Two-Tier Battery Protection

## D1: Power Latching & 1000ms Boot Filter

When the physical button is pressed, VCC turns on hardware rails. `setup()` runs immediately:
1. `pinMode(POWER::POWER_ENABLE, OUTPUT)` and `pinMode(POWER::POWER_BUTTON, INPUT)`.
2. Boot loop waits for `POWER_BUTTON` to remain `HIGH` for `1000ms` total from initial boot.
3. If button goes `LOW` before `1000ms`, loop exits without setting `POWER_ENABLE` `HIGH`. Button release powers off board.
4. If button stays `HIGH` for `1000ms`, `digitalWrite(POWER::POWER_ENABLE, HIGH)` latches power ON.

```
┌──────────────────────────────────────────────────────────┐
│                      BOOT LATCH FLOW                     │
├──────────────────────────────────────────────────────────┤
│ setup()                                                  │
│   pinMode(POWER_ENABLE, OUTPUT); digitalWrite(LOW);      │
│   pinMode(POWER_BUTTON, INPUT);                          │
│   start = millis();                                      │
│   while (digitalRead(POWER_BUTTON) == HIGH) {            │
│       if (millis() - start >= 1000) {                    │
│           digitalWrite(POWER_ENABLE, HIGH); // Latched   │
│           break;                                         │
│       }                                                  │
│   }                                                      │
└──────────────────────────────────────────────────────────┘
```

## D2: Runtime 4000ms Hold & Power Off

`HardwareInit` provides `latchPower()`, `updatePowerButton()`, and `powerOff()`:
- `updatePowerButton()` tracks `s_powerButtonHoldStart`.
- If button is `HIGH` continuously for `>= 4000ms`:
  - `powerOff()`: calls `HardwareInit::stopAll()`, `AudioOutput::stop()`, and `digitalWrite(POWER::POWER_ENABLE, LOW)`.

## D3: Two-Tier Battery Protection

`HardwareConfig::Battery` fields:
- `cellCount`: 1..4
- `warningVoltage`: default 3.5V/cell
- `cutoffVoltage`: default 3.3V/cell
- `vScale`, `vOffset`

State transitions in `VehicleController::update()`:
- `batV < warningVoltage * cellCount` → set `s_batteryWarning = true` (audio/telemetry alert).
- `batV < cutoffVoltage * cellCount` for `>= 1500ms` → `HardwareInit::powerOff()` (stops outputs & drives `POWER_ENABLE` `LOW`).

## D4: Config Schema Integration

`configs/schemas/hardware_config.schema.json` update:
- `warning_voltage`: number, default 3.5, minimum 3.0, maximum 4.0
- `cutoff_voltage`: number, default 3.3, minimum 3.0, maximum 3.8
- `ConfigParser.h` sets defaults if keys omitted.
