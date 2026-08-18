## 1. Battery Protection Implementation

- [x] 1.1 Add automatic LiPo cell count detection (2S, 3S, 4S) in `VehicleController::init()` by sampling startup voltage.
- [x] 1.2 Implement 1.5-second low-voltage monitoring loop in `VehicleController::update()` with 3.3V/cell cutoff threshold.
- [x] 1.3 Trigger motor drive disable, hazard light flashing, and `outOfFuel` audio alert upon low-voltage cutoff.

## 2. Advanced Lighting Automation

- [x] 2.1 Add steering auto-turn-signal logic in `VehicleController::update()` (active when |steer| > 35%, cancels when |steer| < 10%).
- [x] 2.2 Implement dynamic deceleration brake light activation when throttle drops rapidly (> 30% per cycle).
- [x] 2.3 Add 3-state headlight stepping (Off, 40% Low Beam, 100% High Beam) and synchronized hazard light flashing.

## 3. Engine Start & Sound FX Integration

- [x] 3.1 Implement Engine Start/Stop power state in `VehicleController` and `RcEngineSound`, defaulting to `OFF` at boot.
- [x] 3.2 Wire RadioKit engine start trigger to play cranking audio sequence before enabling drive motor and idle loop.
- [x] 3.3 Implement physics-based auto-triggers in `VehicleController::update()` for Jake Brake (high RPM decel) and Turbo Wastegate blow-off (rapid rev drop).

## 4. Verification and Build

- [x] 4.1 Verify code compilation using `pio run -e TRACKLINK_V3`.
- [x] 4.2 Validate hot-reloading and safety cutoff integration.
