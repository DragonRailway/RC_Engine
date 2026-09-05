## 1. ESP32_EasyKit MCPWM Clock Frequency Compensation

- [x] 1.1 Add `getMcpwmTimerResolution` inline helper in `ESP32_EasyKit/src/common/mcpwm_manager.h` to dynamically calculate compensated `resolution_hz`
- [x] 1.2 Update `EasyServo::attach()` in `ESP32_EasyKit/src/EasyServo.cpp` to use `getMcpwmTimerResolution` for `timer_cfg.resolution_hz`
- [x] 1.3 Update `EasyMotor::begin()` in `ESP32_EasyKit/src/EasyMotor.cpp` to use `getMcpwmTimerResolution` for `timer_cfg.resolution_hz`

## 2. RC_brain Integration & Build Configuration

- [x] 2.1 Verify `RC_brain` picks up the updated `ESP32_EasyKit` changes
- [x] 2.2 Compile `MIKRO_V2_SWEEP` test target (`pio run -e MIKRO_V2_SWEEP`)

## 3. Hardware Sweep & Vehicle Verification

- [x] 3.1 Flash `MIKRO_V2_SWEEP` to `/dev/ttyACM0` and verify the steering servo sweeps smoothly between 80° and 100° at 80 MHz CPU
- [x] 3.2 Flash full `MIKRO_V2` firmware and verify steering and drive motor operation at 80 MHz CPU on hardware
