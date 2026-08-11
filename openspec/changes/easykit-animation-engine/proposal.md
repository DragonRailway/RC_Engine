## Why

The vendored ESP32_EasyKit library ships a complete non-blocking animation engine — asymmetric-sigmoid servo easing, LED fade curves, and pattern blink state machines — but RC_brain only uses its instant-write surface (`writeMicroseconds`, `write`), and the main loop never calls `EasyServo::update()`/`EasyLED::update()`, so every animated capability is dormant. Meanwhile `VehicleController` hand-rolls light timing with a hardcoded 333 ms flash that contradicts the config's declared `interval_on`/`interval_off` (500/500 ms), and headlight/aux-servo outputs snap between states. Waking the animation engine removes the re-implemented logic, honors existing config fields, and adds smooth motion at zero hardware cost (no extra MCPWM/LEDC channels).

## What Changes

- **Update pump**: `HardwareInit` gains an `update()` that fans out to every `EasyServo` and `EasyLED` animation engine, called from the main loop so easing/fade/blink advance non-blocking.
- **Servo easing**: Aux servos (dump bed, mixer, excavator arm) move through `write(µs, speed, kIn, kOut)` with config-driven speed and easing strength. Steering stays instant for RC feel.
- **LED fades**: Headlight 3-state stepping (Off → 40% → 100%) transitions via `fadeTo()` over a configurable duration instead of snapping.
- **Config-driven blink**: Turn signals and hazards use `EasyLED.startBlink(onMs, offMs, duty)`, reading `interval_on`/`interval_off`/`brightness_max` from the hardware config. The hand-rolled `(millis() / 333) % 2` logic in `VehicleController` is deleted. **BREAKING** (behavioral): default turn/hazard flash rate changes from the hardcoded 1.5 Hz (333 ms) to the config-declared interval (500/500 ms → 1 Hz on both shipped configs).
- **Config plumbing**: New `lower_snake_case` hardware-config JSON fields for animation tunables (easing speed/kIn/kOut, fade duration), parsed by `ConfigParser` into new `HardwareConfig` members with sensible defaults for configs that omit them.
- **Not breaking**: `HardwareInit::setMotor`, `setServo`, `setLight` signatures stay unchanged.

## Capabilities

### New Capabilities
- `easykit-animation-engine`: Non-blocking animated output on top of the vendored EasyKit library — servo easing, LED fades, config-driven blink patterns, the polling `update()` pump, and the hardware-config tunables that drive them.

### Modified Capabilities
- `advanced-lighting-automation`: Turn-signal/hazard flash timing changes from a hardcoded 1.5 Hz to the config-defined `interval_on`/`interval_off`.
- `config-filesystem-management`: Hardware-config parsing gains the new animation tunable fields (the schema remains `lower_snake_case`; the new fields are optional with defaults).

## Impact

- `common/Config.h` — `HardwareConfig` struct gains animation fields.
- `common/ConfigParser.h` — parse the new fields (optional, defaults).
- `common/HardwareInit.h` — `update()` pump, easing/fade/blink output paths.
- `common/VehicleController.h` — call the pump; replace hand-rolled blink with `startBlink`; fade headlight stepping; ease aux servos.
- `src/main.cpp` — main loop calls `HardwareInit::update()`.
- `data/hardware-TRACKLINK_V3.json`, `data/hardware-MIKRO_V2.json` — declare animation tunables.
- `lib/ESP32_EasyKit` — consumed, not modified.
- Resource budget: unchanged (blink/fade/easing reuse the already-allocated MCPWM operators and LEDC channels).
