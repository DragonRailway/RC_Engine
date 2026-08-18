## Why

A gap audit of the archived specs against the firmware found two small but real gaps:

- `vehicle-control-loop` requires that `brake_pedal` "reduces/brakes the drive motor output proportionally to the pedal position" — but today the pedal only plays the brake sound and lights the brake lamp; the motor keeps running at full throttle. This is the one spec scenario the firmware fails.
- The sound engine already exposes `triggerIndicator` and every vehicle config has an indicator volume, yet no code ever calls it — the automatic turn signals from `advanced-lighting-automation` are silent.

Both are small, tightly-scoped polish fixes that close spec gaps without touching the UI.

## What Changes

- **Proportional brake pedal motor braking**: `brake_pedal` above a 20% deadband scales the drive motor output linearly down to zero at full brake, applied to both Ackermann (single motor) and skid-steer (differential) drivetrains. The sound engine RPM continues to follow the raw throttle so braking doesn't stall the engine simulation.
- **Indicator click sound**: `triggerIndicator` plays while any turn indicator is active — automatic steering turn signals or manual hazard flashing.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `vehicle-control-loop`: "Direction and braking" requirement — motor braking behavior made precise (linear blend, deadband, skid-steer coverage, engine-simulation independence).
- `advanced-lighting-automation`: added indicator click sound requirement (sound follows active turn indicators).

## Impact

- `common/VehicleController.h` — brake blend in the motor path; indicator sound trigger with change detection. No new widgets, config schema, or board changes.
- Sound engine, configs, UI design: unchanged (assets and volumes already exist).
