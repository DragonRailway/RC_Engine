## 1. Incandescent Lighting Slew Engine & Ditch Light Cross-Fade

- [x] 1.1 Implement asymmetric software PWM slew rate limiter for locomotive light channels (`head_light`, `tail_light`, `cab_light`, `step_light`) in `RC_brain/common/VehicleController.h`.
- [x] 1.2 Implement smooth triangular PWM cross-fading for dual ditch lights (`L4`, `L5`) in `RC_brain/common/VehicleController.h`.
- [x] 1.3 Add automatic ditch light cross-fade activation when bell (`bell_button`) or horn triggers are active.

## 2. Directional Lighting Coupling

- [x] 2.1 Couple `head_light` and `tail_light` power to the reverser switch (`dir_switch`) state in locomotive mode.
- [x] 2.2 In Forward (`1`), drive forward headlight at full target brightness and dim/extinguish tail marker; in Reverse (`0`), drive rear marker at full target brightness and dim/extinguish forward headlight.

## 3. Reverser Interlock & Throttle Zeroing

- [x] 3.1 Implement reverser flip detection in `VehicleController.h` when `dir_switch` changes while `speed > 0`.
- [x] 3.2 Clamp throttle demand to 0 and apply dynamic braking until locomotive comes to a complete standstill (`speed == 0`).
- [x] 3.3 Delay reversing motor bridge polarity until stationary, then allow new throttle inputs.

## 4. Verification & Testing

- [x] 4.1 Run host DSP / vehicle controller test scripts in `RC_brain/scripts/`.
- [x] 4.2 Rebuild and upload `TRACKLINK_V3` firmware to the board.
- [x] 4.3 Verify directional lighting, ditch cross-fading, and reverser interlock via remote API and live testing.
