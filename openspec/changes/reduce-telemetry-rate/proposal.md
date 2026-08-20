## Why

BLE widget commands experience progressive lag (32ms idle → 79ms under load) because the firmware publishes telemetry VAR_UPDATE frames at 10Hz (20 frames/sec), consuming 60-100% of available BLE notification bandwidth on a typical 30ms connection interval. Command ACKs are delayed or dropped when the TX queue is saturated.

## What Changes

- **Telemetry rate reduction**: Change `s_lastTelemetry` threshold from 100ms to 1000ms in `VehicleController::updateTelemetry()`, reducing BLE telemetry traffic from 20 frames/sec to 2 frames/sec.
- **No behavioral change**: Battery percentage and speed are status displays — 1Hz updates are visually smooth and technically sufficient. The Serial `[STATUS]` debug output remains at 10Hz (USB CDC, not BLE).

## Capabilities

### New Capabilities

_(none — this is a tuning change, not a new capability)_

### Modified Capabilities

- `vehicle-control-loop`: Update the telemetry publishing rate requirement from 10Hz to 1Hz. The spec currently states telemetry is published "periodically" — the rate is now explicitly 1Hz.

## Impact

- **Firmware**: `common/VehicleController.h` — single constant change (`100` → `1000`)
- **BLE bandwidth**: Frees ~80% of notification slots for command ACKs and widget updates
- **App telemetry display**: Updates at 1Hz instead of 10Hz (no perceptible difference for battery/speed)
- **Serial debug**: Unchanged — `[STATUS]` lines still print at 10Hz via USB CDC
