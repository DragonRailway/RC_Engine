## Why

The hardware configurations currently specify `"fade_duration_ms": 250`. Because `EasyLED` applies a sigmoid (ease-in-out) curve over 250ms, LEDs remain at <3% duty for the first 60ms and take over 120ms to become visibly illuminated. This creates a noticeable visual lag when toggling headlights or full beams.

## What Changes

- **Instant Hardware Light Switching**: Change `"fade_duration_ms"` from `250` to `0` (or `30` ms) across `hardware-*.json` and default configs, enabling immediate LED response when toggling light states.

## Capabilities

### Modified Capabilities
- `advanced-lighting-automation`: Update headlight and full beam transition requirements to instantaneous (<30ms) switching.

## Impact

- **Firmware Files**: `configs/hardware_configs/hardware-*.json`.
- **Latency**: Removes 250ms sigmoid fade delay on physical LEDs.
