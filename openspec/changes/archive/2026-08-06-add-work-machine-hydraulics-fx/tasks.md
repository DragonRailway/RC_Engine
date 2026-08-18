## 1. Hardware & Control Infrastructure

- [x] 1.1 Add auxiliary servo drivers (`auxServo1`, `auxServo2`) in `HardwareInit.h` mapped to Servo 2 and Servo 3 pins via `EasyServo`.
- [x] 1.2 Expose auxiliary control variables (`aux_hydraulic1`, `aux_hydraulic2`, `bucket_rattle_trigger`, `dump_bed_toggle`) in `VehicleController.h`.

## 2. Sound Engine & Load Governor Evaluation

- [x] 2.1 Implement hydraulic flow detection (> 10% magnitude) in `VehicleController::update()` triggering `s_engine->triggerHydraulicFlow()`.
- [x] 2.2 Implement +20% engine idle RPM pump load governor bump when hydraulic flow is active.
- [x] 2.3 Implement speed-dependent track pin rattle evaluation (`s_engine->triggerTrackRattle()`) for tracked vehicles (`features.trackRattleEnabled`).
- [x] 2.4 Wire `bucket_rattle_trigger` to trigger `s_engine->triggerBucketRattle()`.

## 3. Physical Servo Mapping & Verification

- [x] 3.1 Map `aux_hydraulic1` and `aux_hydraulic2` values (-100..+100) to physical auxiliary servo output pulse widths (1000us..2000us).
- [x] 3.2 Verify code compilation using `pio run -e TRACKLINK_V3`.
