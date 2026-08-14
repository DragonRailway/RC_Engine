# Hybrid Two-Layer Hardware & Inertia Verification Suite

This document describes the testing framework for verifying motor PWM control, Park lock safety, light automation, flywheel inertia simulation, and firmware stability for the ESP32-S3 RC Vehicle Controller.

---

## Overview

The verification system uses a **Hybrid Two-Layer Architecture**:

1. **Layer 1: Host x86 Simulation & Physics Harness (`scripts/host_vc_test.py`)**
   - Compiles `VehicleController.h`, `RcEngineSound.cpp`, and mock hardware drivers natively on x86 C++.
   - Executes deterministic 10ms simulation ticks.
   - Asserts zero motor PWM in Park lock (`gear=1`), proportional brake blending (`brake_pedal > 20%`), reverse direction inversion, RPM acceleration/deceleration inertia curves (`acc`/`dec`), Jake brake drag physics, and hydraulic load governor (+20% idle RPM boost).

2. **Layer 2: Live On-Hardware Telemetry & Panic Suite (`scripts/hardware_verification.py`)**
   - Connects to physical ESP32 hardware over USB Serial (`/dev/ttyACM0` @ 2,000,000 baud).
   - Drives RadioKit widget command frames end-to-end.
   - Verifies 100% RadioKit ACK receipt.
   - Monitors live serial output and asserts zero `Guru Meditation`, `Coprocessor exception`, `abort()`, or backtrace crashes during heavy state transitions.

---

## Test Execution Commands

### 1. Run Host Layer 1 Physics Harness
```bash
python3 scripts/host_vc_test.py
```

### 2. Run Live Hardware Layer 2 Suite
```bash
python3 scripts/hardware_verification.py
```

### 3. Run Full Hybrid Suite
```bash
python3 scripts/host_vc_test.py && python3 scripts/hardware_verification.py
```
