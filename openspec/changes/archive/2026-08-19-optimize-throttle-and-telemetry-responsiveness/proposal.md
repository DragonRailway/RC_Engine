## Why

Currently, throttle interactions from the RadioKit mobile/tablet control surface exhibit a noticeable delay. Analysis of the firmware and communication stack revealed three primary contributors:
1. **Engine Audio Simulation Delay**: The automatic transmission inertia model (`RcEngineSound.cpp`) increments virtual speed by only 4 RPM per update cycle, requiring 126 loop cycles to transition through 3 gears from idle to top speed.
2. **Telemetry Update Quantization**: Speed and battery telemetry are throttled to a 250ms cadence, adding up to 250ms of visual and feedback latency.
3. **BLE Control Loop Responsiveness**: Main loop pacing and status updates can be tuned for low-latency command ingestion.

## What Changes

- **Sound Engine Acceleration Tuning**: Increase the default virtual speed step in `RcEngineSound.cpp` for automatic transmission vehicles and tune `ScaniaV8` acceleration parameters for immediate audio feedback.
- **Dynamic Fast-Path Telemetry Interval**: Reduce the telemetry publication interval in `VehicleController.h` from 250ms to 100ms when connected over active transports to ensure real-time speedometer and gauge tracking.
- **Direct Throttle Coupling**: Ensure physical motor drive and audio state updates execute on the zero-latency path without blocking delays.

## Capabilities

### Modified Capabilities
- `vehicle-control-loop`: Lower telemetry publication interval to 100ms and tune throttle acceleration dynamics for responsive sound and drive tracking.
- `radiokit-ble-control`: Ensure high-frequency variable updates are consumed immediately with minimal latency.

## Impact

- **Firmware Files**: `common/VehicleController.h`, `lib/SoundEngine/src/RcEngineSound.cpp`, `configs/vehicle_configs/ScaniaV8/vehicle.json`.
- **Performance**: Instant throttle response, real-time speed telemetry at 10 Hz (100ms), and snappy engine audio transitions.
