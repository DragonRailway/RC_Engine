## Why

While the audio synthesis pipeline has extensive test tooling, the vehicle control physics (RPM acceleration/deceleration inertia curves, flywheel mass, Jake brake drag, hydraulic governor boost) and physical hardware outputs (drive motor PWM duty, skid-steer differential mixing, 3-state headlights, auto turn-signal cancel, LiPo low-voltage cutoff) require dedicated, automated verification.

Building a hybrid two-layer test suite guarantees that both the off-device state-machine math (Layer 1 x86 simulation) and on-device hardware peripherals (Layer 2 live serial telemetry) behave deterministically across all vehicle profiles and board targets without panics or timing regressions.

## What Changes

- **Layer 1 (Off-Device C++ Physics Harness)**: Create an x86 simulation harness in `test/host_vc/` to assert `VehicleController` math across simulated 10ms ticks:
  - RPM acceleration/deceleration curves (`acc`, `dec`, `inertia`).
  - Park Lock zero-PWM safety, proportional brake blending (`brake_pedal > 20%`), and skid-steer differential mixing.
  - Light automation (3-state headlights, dynamic deceleration brake trigger, auto turn-signal cancel).
  - Hydraulic governor RPM bump (+20%) and LiPo low-voltage cutoff debounce (1.5s).
- **Layer 2 (Live Hardware Telemetry Suite)**: Extend `scripts/smoke_test.py` and create `scripts/hardware_verification.py` to drive connected hardware over `/dev/ttyACM0` @ 2 Mbaud, parse live serial telemetry, and assert 0 panic crashes.
- **Documentation**: Document the workflow and command references in `docs/hardware_inertia_verification.md`.

## Capabilities

### New Capabilities
- `hardware-verification`: Automated test coverage for drive motors, steering/aux servos, 3-state headlights, indicators, brake lights, and battery protection.
- `inertia-simulation-verification`: Automated test coverage for engine RPM ramp curves, virtual flywheel inertia, Jake brake deceleration drag, and hydraulic load governor.

### Modified Capabilities
(None)

## Impact

- **Codebase**: Creates `test/host_vc/` harness, adds `scripts/hardware_verification.py`, updates `scripts/smoke_test.py`.
- **APIs / Contracts**: No breaking changes to `VehicleController.h` or `RadioKit` protocol.
- **Hardware/Target Compatibility**: Supports `TRACKLINK_V3` and `MIKRO_V2` ESP32-S3 boards.
