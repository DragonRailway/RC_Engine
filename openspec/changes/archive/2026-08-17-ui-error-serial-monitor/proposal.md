## Why

When firmware encounters configuration validation warnings, syntax/schema errors, or runtime faults (e.g. battery cutoff or missing components), operators currently need an active USB serial monitor connection to observe log output. Adding a dynamic UI serial monitor directly in the RadioKit control app allows instant on-screen error diagnostics on both Truck and Loco pages while remaining hidden during normal error-free operation.

## What Changes

- Update `RADIOKIT.h` to include `serial_monitor_1` (Truck page) and `serial_monitor_2` (Loco page), configured hidden by default.
- Introduce `UiLogger` (or error bridge in `common/`) to capture and format config errors/warnings and runtime faults.
- Buffer boot-time warnings during initial `ConfigParser` execution prior to RadioKit BLE startup, and flush them to UI serial monitors upon init.
- Automatically unhide (`setHidden(false)`) both `serial_monitor_1` and `serial_monitor_2` when errors or warnings are logged.
- Print error messages to the UI serial monitor widgets only.
- Auto-hide (`setHidden(true)`) and clear monitors when a valid configuration is successfully hot-reloaded without warnings.

## Capabilities

### New Capabilities
- `ui-error-monitor`: Captures boot/hot-reload config warnings and runtime errors, manages dynamic visibility of UI serial monitor widgets across all control pages, and auto-hides on clean recovery.

### Modified Capabilities
<!-- No requirement changes to existing capabilities -->

## Impact

- `src/RADIOKIT.h`: Updated from latest designer surface with `serial_monitor_1` and `serial_monitor_2`.
- `src/main.cpp`: Integrate `UiLogger` boot queue flush, hot-reload error clearance, and monitor visibility management.
- `common/ConfigParser.h`: Route config parse warnings to `UiLogger` for UI presentation.
- `common/VehicleController.h` / runtime error sites: Route runtime faults to `UiLogger`.
