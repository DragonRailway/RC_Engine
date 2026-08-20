## Context

Light fading with 250ms sigmoid curve introduces ~150ms of imperceptible duty progression before LEDs light up. Setting fade duration to 0ms (or 30ms) makes LED activation crisp and immediate.

## Goals / Non-Goals

**Goals:**
- Update `fade_duration_ms: 0` in all hardware configs and parser defaults.

**Non-Goals:**
- Removing `setLightFade` capability from EasyLED.

## Decisions

- Set `"fade_duration_ms": 0` in `hardware-MIKRO_V2-truck.json`, `hardware-MIKRO_V2-skid.json`, and `hardware-TRACKLINK_V3-locomotive.json`.
