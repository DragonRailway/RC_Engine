# Design: Board Power & Charging Visual Indicators

## D1: Schema & Data Structures

`HardwareConfig` in `common/Config.h`:

```cpp
struct Charging {
    uint8_t pin = 0xFF;         // Pin token or aliased light pin
    uint8_t mode = 0;           // 0=solid, 1=blink, 2=pulse
    bool    configured = false;
} charging;

struct Power {
    uint8_t indicatorPin = 0xFF;// Optional power indicator pin / light alias
    float   bootLatchS = 1.0f;
    float   buttonHoldS = 4.0f;
    float   disconnectTimeoutS = 60.0f;
    float   warningWindowS = 10.0f;
    float   cutoffDelayS = 1.5f;
} power;
```

---

## D2: Light Alias & Pin Resolution in `ConfigParser.h`

In `ConfigParser.h`:
```cpp
// Resolve pin or light alias for charging indicator
if (!docObj["charging"].isNull()) {
    JsonObjectConst chgObj = docObj["charging"];
    const char* hw = chgObj["hardware"] | "";
    config.charging.pin = PinMapper::resolve(hw);
    if (config.charging.pin == 0xFF && hw[0] != '\0') {
        // Resolve light aliases (head_light, tail_light, cab_light, etc.)
        config.charging.pin = resolveLightAlias(hw, config);
    }
    const char* modeStr = chgObj["mode"] | "solid";
    if (strcasecmp(modeStr, "blink") == 0) config.charging.mode = 1;
    else if (strcasecmp(modeStr, "pulse") == 0) config.charging.mode = 2;
    else config.charging.mode = 0;
    config.charging.configured = (config.charging.pin != 0xFF);
}
```

---

## D3: Button-Hold Rapid Blink Feedback Flow

In `HardwareInit::updatePowerButton(float buttonHoldS)`:

```
  POWER_BUTTON HIGH
        │
        ▼
  s_powerButtonHolding == true
        │
        ├─────────────────────────────────────────┐
        ▼                                         ▼
  now - s_powerButtonHoldStart < holdMs     now - s_powerButtonHoldStart >= holdMs
        │                                         │
        ▼                                         ▼
  Blink indicator pin (200ms ON / 200ms OFF)   Indicator OFF -> powerOff()
```

---

## D4: Charging State Indicator Animation

In `VehicleController::update()`:
When `HardwareInit::isCharging()` is true:
- Set motor outputs to zero.
- If `config.charging.configured`:
  - Mode 0 (`solid`): drive indicator LED to 100%.
  - Mode 1 (`blink`): drive indicator LED with 500ms blink interval.
  - Mode 2 (`pulse`): drive indicator LED with sine-wave PWM breathing effect.
