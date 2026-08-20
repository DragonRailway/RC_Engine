## Context

Hazard and turn blinking were running with 500ms ON / 500ms OFF (1.0 Hz). This made hazard flashing feel slow when activated from the UI and created phase desynchronization with the vehicle engine sound click sample (~398ms).

## Goals / Non-Goals

**Goals:**
- Update default turn/hazard flasher interval to 300ms ON / 300ms OFF across `Config.h`, `ConfigParser.h`, and `hardware-*.json`.
- Provide responsive 1.67 Hz flasher cadence aligned with real-vehicle automotive standards.

**Non-Goals:**
- Rewriting the EasyLED blink state machine.

## Decisions

- **Decision**: Standardize `interval_on: 300` and `interval_off: 300` in `hardware-MIKRO_V2-truck.json`, `hardware-MIKRO_V2-skid.json`, and `hardware-TRACKLINK_V3-locomotive.json`, and update the struct/parser defaults.
- **Rationale**: 300ms ON / 300ms OFF gives 1.67 Hz (100 flashes/min), which looks crisp, responds quickly upon toggle, and closely matches the sound duration.
