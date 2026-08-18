## 1. Firmware Telemetry Instrumentation

- [x] 1.1 Add real-time event & periodic serial debug logging to `VehicleController` (`common/VehicleController.h`) for engine state, gear, throttle, brake blend, steering angle, and lights.
- [x] 1.2 Verify compilation for `MIKRO_V2` environment (`pio run -e MIKRO_V2`).

## 2. Flash Deployment

- [x] 2.1 Flash compiled firmware binary to `MIKRO_V2` board via `/dev/ttyACM0` (`pio run -e MIKRO_V2 -t upload`).
- [x] 2.2 Build and flash the LittleFS bundle with `hardware-MIKRO_V2-truck.json` and `ScaniaV8` vehicle profile (`python3 scripts/build_fs.py --board MIKRO_V2 --hardware truck --vehicle ScaniaV8`).
- [x] 2.3 Verify boot sequence, LittleFS mounting, and config/sound JSON loading over serial monitor.

## 3. End-to-End Test Suite & Remote Verification

- [x] 3.1 Create unified verification script `scripts/verify_mikro_e2e.py` driving the RadioKit REST API (`127.0.0.1:17007`) while capturing serial logs.
- [x] 3.2 Execute and assert Engine simulation phases (Start toggle, idle loop, acceleration ramp, Jake Brake, Horn).
- [x] 3.3 Execute and assert Drivetrain phases (Gears D/P/R, Throttle, Proportional Brake blend, Steering endpoints & Auto-turn signals).
- [x] 3.4 Execute and assert Lighting phases (Headlights 3-state Off/Low/High, Hazards, Reverse lights, Decel brake lights).
- [x] 3.5 Execute and assert Work Machine Aux / Hydraulics channel and Audio DSP performance (< 2900 µs buffer time, no NaNs/clipping).
