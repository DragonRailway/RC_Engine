## 1. Animation Config Schema

- [x] 1.1 Add `HardwareConfig::Animation` struct (easingSpeedDegS, easingKIn, easingKOut, fadeDurationMs) with defaults (180, 0.2, 0.8, 250) to `common/Config.h`
- [x] 1.2 Parse the optional `"animation"` block (`easing_speed_deg_s`, `easing_k_in`, `easing_k_out`, `fade_duration_ms`) in `ConfigParser::loadHardwareConfig`, keeping defaults when absent
- [x] 1.3 Add an explicit `"animation"` block to `data/hardware-TRACKLINK_V3.json` and `data/hardware-MIKRO_V2.json`

## 2. HardwareInit Animation Output Paths

- [x] 2.1 Add `HardwareInit::update()` that fans out to `steeringServo.update()`, `escServo.update()`, `auxServo1.update()`, `auxServo2.update()`, and `update()` on every attached EasyLED
- [x] 2.2 Capture the animation config into HardwareInit statics during `init()`/`hotReload()`
- [x] 2.3 Add `HardwareInit::setLightBlink(pin, active, onMs, offMs, dutyPct)` — edge-triggered `startBlink`/`stopBlink` via `findLight`
- [x] 2.4 Add `HardwareInit::setLightFade(pin, targetPct, durationMs)` — `fadeTo` with percent→raw-tick conversion (`getMaxDuty() * pct / 100`), EASE_IN_OUT curve, stopping any active blink first
- [x] 2.5 Switch `setAuxServo1`/`setAuxServo2` to eased `write((float)us, speed, kIn, kOut)` when `easingSpeedDegS > 0`, else keep `writeMicroseconds`
- [x] 2.6 Make `stopAll()` fully stop animation state: `stop()` on every LED (cancels blink/fade/breathing) — servos are covered by `detach()`
- [x] 2.7 Expose live duty read-back (e.g. `HardwareInit::getLightDutyPercent(pin)` via `findLight`→`getDutyPercent`) for the tail-light tracking

## 3. VehicleController Integration

- [x] 3.1 Replace the hand-rolled `(millis() / 333) % 2` turn/hazard flashing with `setLightBlink` edges using `turn_light.interval_on`/`interval_off`/`brightness_max`; stop calling `setLight` on the turn pins
- [x] 3.2 Drive headlight 3-state stepping (Off/40%/100%) through `setLightFade` with `fade_duration_ms`; only trigger on state change
- [x] 3.3 Derive the tail light level from the headlight's live duty read-back instead of the target brightness
- [x] 3.4 Stop all blinks in the battery-cutoff branch (so a hazard/turn blink cannot strand an LED on)
- [x] 3.5 Remove the now-unused decel/hazard flash timing code and any dead `interval` duplication

## 4. Main Loop Pump

- [x] 4.1 Call `HardwareInit::update()` every iteration in `src/main.cpp` `loop()` (after `VehicleController::update()`)

## 5. Validation

- [x] 5.1 Build both environments: `pio run -e TRACKLINK_V3` and `pio run -e MIKRO_V2`
- [x] 5.2 Run `scripts/smoke_test.py` end-to-end (flash + boot + control assertions)
- [x] 5.3 Bench-verify on hardware: eased aux-servo sweep, headlight fade transition, turn blink at configured interval (via RadioKit config upload + hot-reload)
