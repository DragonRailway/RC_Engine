## Why

The RadioKit control UI features real-time telemetry display cards for battery and speed, but the speed widget is currently unscaled and the battery percentage drops to 0% only at emergency cutoff rather than alerting the driver at the low-voltage warning threshold. We need to wire both telemetry widgets to provide intuitive, realistic feedback to the driver.

## What Changes

- **Battery Percentage (0-100%) with Warning Floor**: Re-scale the battery percentage display so that 0% corresponds to the configured `warning_voltage` (e.g. 3.5V/cell) and 100% corresponds to `full_voltage` (4.2V/cell). Any voltage at or below the warning threshold is clamped to 0%.
- **Speed in km/h (0-200 km/h)**: Wire `telemetry_Speed` to map absolute motor drive output (0-100%) to a 0-200 km/h range (`abs(motorSpeed) * 2`).
- **Telemetry Frequency**: Maintain the stable 250ms (4 Hz) update cycle to synchronize battery and speed strings with the active RadioKit connection.

## Capabilities

### Modified Capabilities
- `battery-protection`: Battery telemetry percentage floor changes from `cutoff_voltage` to `warning_voltage` so that 0% is displayed at or below the warning threshold.
- `radiokit-ble-control`: Telemetry stream transmits `Battery` in percent (0-100%) and `Speed` in km/h (0-200 km/h).

## Impact

- `common/VehicleController.h`: `updateTelemetry()` logic updated for warning-floor battery percentage and 0-200 km/h speed calculation.
- `docs/radiokit-rc-ui-design.json`: Verified telemetry schema metadata (`unit: "%"` for Battery, `unit: "km/h"` for Speed).
- `test/host_vc/host_vc_driver.cpp`: Unit test assertions updated to verify warning-level 0% clamping and speed scaling.
