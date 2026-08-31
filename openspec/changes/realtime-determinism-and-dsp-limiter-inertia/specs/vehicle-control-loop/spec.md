## ADDED Requirements

### Requirement: Deterministic 50 Hz Periodic Control Task
The firmware SHALL execute the main vehicle control loop (`VehicleController::update()`) and peripheral animation update (`HardwareInit::update()`) inside a dedicated periodic FreeRTOS task running at a strict 50 Hz (20.0 ms) period:
1. The task SHALL use `vTaskDelayUntil()` to eliminate timing jitter caused by BLE packet processing and serial telemetry.
2. The task SHALL be pinned to Core 1 at priority 2, running below the real-time audio task (priority 3) and above the base Arduino `loopTask` (priority 1).

#### Scenario: Steady 50 Hz loop period during high BLE traffic
- **WHEN** the BLE transport receives high-bandwidth telemetry notifications or control packets
- **THEN** `VehicleController::update()` executes at steady 20.0 ms intervals without timing jitter or frame drops

### Requirement: Event-driven configuration reload
The firmware SHALL reload configuration files upon receiving an explicit filesystem upload completion event from RadioKit, eliminating periodic filesystem `stat()` polling:
1. The main loop SHALL NOT execute periodic `stat()` or `fileWriteTime()` checks against LittleFS during normal operation.
2. When a file upload completes, the reload handler SHALL execute safely to hot-reload hardware and vehicle configs.

#### Scenario: File upload triggers instant reload
- **WHEN** a new `vehicle.json` or `hardware.json` is uploaded over RadioKit
- **THEN** the firmware reloads the configurations without requiring periodic timer polling
