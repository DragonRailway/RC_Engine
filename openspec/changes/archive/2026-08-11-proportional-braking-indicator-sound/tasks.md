## 1. Proportional Brake Pedal Braking

- [x] 1.1 Compute `brakePct` from `brake_pedal.rk.value` and derive a blended `motorThrottle` (linear scale above 20% deadband) in `VehicleController::update()`
- [x] 1.2 Apply `motorThrottle` to the Ackermann motor output and to both skid-steer sides (before differential mixing)
- [x] 1.3 Keep the sound engine RPM and light automation on the raw `throttlePct` (brake does not stall the engine simulation)

## 2. Indicator Click Sound

- [x] 2.1 Add an `s_indicatorPrev` static and compute the effective indicator state (`hazardActive || s_autoTurnLeft || s_autoTurnRight`)
- [x] 2.2 Call `s_engine->triggerIndicator(active)` with change detection each update

## 3. Validation

- [x] 3.1 Build `pio run -e TRACKLINK_V3` and `pio run -e MIKRO_V2` with no errors
- [x] 3.2 Review the diff against the delta specs (each scenario has matching behavior in code)
