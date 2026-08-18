## Context

The `RC_brain` vehicle control system (`common/VehicleController.h`, `common/HardwareInit.h`) manages physical motor PWM outputs, steering/auxiliary servos, 3-state headlights, turn indicators, brake lights, and low-voltage LiPo protection. Simultaneously, `RcEngineSound.cpp` simulates virtual flywheel inertia (`acc`, `dec`, `inertia`), clutch engagement, Jake brake drag, and hydraulic governor load.

While unit testing has been implemented for audio synthesis, the hardware outputs and physics simulation state machines lack automated, deterministic verification.

## Goals / Non-Goals

**Goals:**
- **Layer 1 (Host x86 Integration Harness)**: Build a standalone C++ driver in `test/host_vc/` that compiles `VehicleController.h` logic on x86 to verify motor PWM duty cycles, Park lock zero-torque safety, proportional brake blending, light state automation, and RPM inertia ramp curves.
- **Layer 2 (Live Hardware Telemetry Suite)**: Extend Python test scripts (`scripts/hardware_verification.py` and `scripts/smoke_test.py`) to connect over serial (`/dev/ttyACM0` @ 2 Mbaud), parse real-time telemetry, and assert zero panic crashes on physical hardware.
- **Documentation**: Provide a detailed guide in `docs/hardware_inertia_verification.md`.

**Non-Goals:**
- Hardware physical bench testing using oscilloscope/logic analyzer probes (we rely on serial telemetry and software state machine assertions).
- Modifying underlying `RadioKit` protocol wire formats.

## Decisions

1. **Decision 1: Native x86 Compilation of VehicleController.h**
   - *Rationale*: `VehicleController.h` and `HardwareInit.h` contain the core hardware mapping and physics logic. By providing lightweight Arduino/LittleFS stubs in `test/host_vc/Arduino.h`, we can run thousands of simulation ticks in milliseconds off-device.
   - *Alternatives Considered*: Running tests exclusively on physical ESP32 boards via `pio test`. Rejected due to long upload times and inability to test all edge cases deterministically.

2. **Decision 2: Two-Layer Verification Strategy**
   - *Rationale*: Layer 1 catches logic bugs and state machine regressions instantly on host; Layer 2 validates real physical microcontrollers (`TRACKLINK_V3` / `MIKRO_V2`), serial protocol ACKs, and boot panics over USB CDC.
   - *Alternatives Considered*: Layer 1 only. Rejected because hardware CDC serial stability and real board initialization must be verified live.

## Risks / Trade-offs

- **[Risk]**: Differences in timer tick resolution between host `millis()` mocks and ESP32 FreeRTOS tasks.
  - *Mitigation*: Advance host virtual time explicitly per tick loop matching 10ms hardware update intervals.
- **[Risk]**: Serial telemetry output volume causing CDC buffer overflow on `-DAUDIO_DEBUG` builds.
  - *Mitigation*: Rate-limit telemetry lines to bounded intervals (e.g. 100ms / 1000ms).
