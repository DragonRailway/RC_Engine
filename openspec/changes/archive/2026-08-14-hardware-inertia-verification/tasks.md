## 1. Host C++ Vehicle Controller & Inertia Harness (Layer 1)

- [x] 1.1 Create `test/host_vc/Arduino.h` mock header for `VehicleController.h` x86 compilation
- [x] 1.2 Write `test/host_vc/host_vc_driver.cpp` to execute `VehicleController::update()` across simulated 10ms ticks
- [x] 1.3 Implement assertions for motor PWM duty cycles, Park lock (0 PWM), proportional brake blending (`brake_pedal > 20%`), and skid-steer differential track mixing
- [x] 1.4 Implement assertions for light automation (3-state headlights, dynamic decel brake trigger, auto turn-signal cancel)
- [x] 1.5 Write `scripts/host_vc_test.py` to compile and run the host VC harness on x86 with exit status reporting

## 2. Physics & Flywheel Inertia Assertions

- [x] 2.1 Implement RPM acceleration & deceleration curve assertions in `host_vc_driver.cpp` matching `cfg.engine.acc` and `inertia`
- [x] 2.2 Implement Jake brake deceleration drag rate assertions
- [x] 2.3 Implement auxiliary hydraulic governor (+20% idle RPM bump) assertions
- [x] 2.4 Implement LiPo battery low-voltage cutoff 1.5s debounce filter assertions (zero motor, 333ms hazard flash, out-of-fuel alert)

## 3. Live Hardware Telemetry & Panic Suite (Layer 2)

- [x] 3.1 Write `scripts/hardware_verification.py` to drive hardware over `/dev/ttyACM0` @ 2 Mbaud
- [x] 3.2 Add live serial telemetry parsing (`[AUDIO_STATS]`, battery voltage, speed, state) and ACK validation to `hardware_verification.py`
- [x] 3.3 Add panic watch assertion (assert zero `Guru Meditation`, `Coprocessor exception`, `abort()`, or backtraces during heavy motor/light transitions)

## 4. Final Validation & Documentation

- [x] 4.1 Run full hybrid suite (`scripts/host_vc_test.py` and `scripts/hardware_verification.py`)
- [x] 4.2 Document the test suite workflow, usage, and command references in `docs/hardware_inertia_verification.md`
