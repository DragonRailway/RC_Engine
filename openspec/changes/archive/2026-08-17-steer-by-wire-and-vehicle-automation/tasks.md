## 1. Steer-by-Wire Continuous Servo Update

- [x] 1.1 Decouple `HardwareInit::setServo(steerVal)` from `eState == RUNNING` in `VehicleController.h` so Ackermann steering is always active
- [x] 1.2 Verify servo response in Park (P) and with engine OFF

## 2. Turn Signal Auto-Cancellation State Machine

- [x] 2.1 Add turn arming tracking flags (`s_leftTurnArmed`, `s_rightTurnArmed`) and mutual exclusion in `VehicleController.h`
- [x] 2.2 Implement auto-cancellation on steering return (left: `< -20%` armed -> `> -8%` cancel; right: `> +20%` armed -> `< +8%` cancel)
- [x] 2.3 Implement immediate cancellation on opposite steering (left active & `steer > +15%`; right active & `steer < -15%`)
- [x] 2.4 Synchronize cancelled toggle states back to RadioKit (`left_indicator.rk.state = false`, `right_indicator.rk.state = false`)

## 3. Engine Start/Stop Gear Interlock

- [x] 3.1 Implement auto-shift to Park (`gear_switch.rk.value = 1`) on engine stop transition
- [x] 3.2 Implement auto-shift to Drive (`gear_switch.rk.value = 0`) on engine start transition if currently in Park
- [x] 3.3 Ensure gear changes trigger shifting sound and sync state to the app UI

## 4. Testing & Verification

- [x] 4.1 Run host tests (`pio test` / `host_vc_test.py`) to verify regressions and test coverage
- [x] 4.2 Build and upload firmware to MIKRO_V2 (`pio run -e MIKRO_V2 -t upload`)
- [x] 4.3 Run end-to-end verification via Remote API and USB serial stream
