## 1. Config & Data Model Updates

- [x] 1.1 Update `VehicleConfig::Engine` in `common/Config.h` with `hasEngine` flag and ensure inertia parameters (`inertia`, `acc`, `dec`, `escRampTime`, `escAccelSteps`, `escBrakeSteps`) are parsed in `common/ConfigParser.h`.
- [x] 1.2 Validate that `ConfigParser.h` cleanly sets `hasEngine = false` when no `"engine"` key exists in the JSON.

## 2. Virtual Mass Inertia State Machine Implementation

- [x] 2.1 Implement `updateMotorInertia()` calculation in `common/VehicleController.h` supporting acceleration ramp, coasting deceleration, and proportional braking.
- [x] 2.2 Add direct mode bypass: if `s_profile == nullptr`, `!s_profile->config.hasEngine`, or `inertia == 0`, pass unramped throttle directly to motor drivers.
- [x] 2.3 Add Park (P=1) and Engine OFF emergency interlocks to instantly reset virtual speed and physical motor duty to 0.

## 3. Drivetrain & Skid-Steer Integration

- [x] 3.1 Route ramped motor speed through Ackermann steering and Skid-Steer differential motor calculations.
- [x] 3.2 Add air-brake sound trigger on coming to a complete stop when transitioning from motion to 0.

## 4. Verification & Testing

- [x] 4.1 Create host unit test in `test/host_vc/` verifying acceleration ramping, coasting, braking, and direct mode fallback.
- [x] 4.2 Run end-to-end verification suite on target board via BLE Remote API and confirm smooth acceleration and 0 noise on stop.
