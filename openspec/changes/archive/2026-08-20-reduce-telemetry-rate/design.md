## Context

The VehicleController publishes `[STATUS]` telemetry at 10Hz via `Serial.printf()` (USB CDC) and updates two `RK_Telemetry` widgets (`telemetry_Battery`, `telemetry_Speed`) which trigger BLE `VAR_UPDATE` frames via the shadow comparison in `RadioKit.update()`. At 10Hz with 2 widgets, this generates 20 BLE notification frames/sec — consuming 60-100% of available bandwidth on a typical 30ms connection interval (~33 notifications/sec max).

## Goals / Non-Goals

**Goals:**
- Reduce BLE telemetry traffic from 20 frames/sec to 2 frames/sec
- Maintain 1Hz battery/speed updates on the app telemetry display
- Keep Serial `[STATUS]` debug output at 10Hz (USB CDC, unrelated to BLE)

**Non-Goals:**
- Changing the Serial debug output rate
- Modifying the RadioKit telemetry widget protocol
- Adjusting the app-side telemetry display refresh rate

## Decisions

- **D1: Threshold change only** — Replace `s_lastTelemetry >= 100` with `s_lastTelemetry >= 1000` in `VehicleController::update()`. This is a single constant change with no structural modification.
- **D2: Serial output unchanged** — The `Serial.printf("[STATUS]...")` call inside `updateTelemetry()` remains at the new 1Hz rate. This is acceptable because the Serial debug output is a secondary concern; the primary goal is BLE bandwidth relief.
- **D3: No config surface** — The telemetry rate is hardcoded, not configurable. Making it configurable would add parser/schema complexity for a tuning constant that rarely needs adjustment.

## Risks / Trade-offs

- [App telemetry updates slower] → 1Hz is visually smooth for battery percentage and speed. No user-perceptible difference.
- [Debugging granularity reduced] → Serial `[STATUS]` lines are now 1Hz instead of 10Hz. Acceptable for production; during development the rate can be temporarily restored.
