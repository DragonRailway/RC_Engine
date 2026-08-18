# EasyKit LED Group (Coordinated Multi-LED Patterns)

## Why

The locomotive hardware config now has a `ditch_light` that must **flash two
outputs alternately** (counter-phased). Today that alternation is hand-rolled
in `VehicleController::applyLightsWithAutomation` (manual `millis()` state:
`s_ditchLastToggle` / `s_ditchSide`), bypassing the animation engine entirely.

But alternating pairs are not a locomotive-only need. Police flashers (double
strobe), hazard lights (sync flash), light bars and rotating beacons (chase),
and grade-crossing signals are all the same underlying abstraction: **a timed
sequence of per-LED duty states**. The EasyKit `EasyLED` blink engine is rich
but strictly single-LED — it has no way to coordinate two or more LEDs.

This change adds a small **step-sequencer group** to the vendored EasyKit lib
(`EasyLEDGroup`) that owns N `EasyLED` members and plays timed patterns
(alternate, sync flash, chase, strobe) against a shared timeline. The ditch
lights migrate onto it; future vehicles get beacon/chase/flasher patterns for
free.

## What

1. **`EasyLEDGroup` class in `lib/ESP32_EasyKit`** — owns a list of `EasyLED`
   members, plays a pattern (a step table: per-step duties + duration) on a
   shared `millis()` timeline, polled via `update()` like the existing engines.
2. **Built-in pattern factories** — `alternate(intervalMs)`, `syncFlash(onMs,
   offMs)`, `chase(intervalMs)`, `doubleStrobe(intervalMs, gapMs)` built on the
   step table.
3. **Ditch-light migration** — the locomotive ditch pair becomes a 2-member
   group driven from the existing app toggle (loco light selector item F);
   `VehicleController` loses its manual alternation state. Config keys
   unchanged (`left`/`right`/`brightness_max`/`interval_ms`).
4. **Pump completeness** — the new light instances (ditch L/R, step, cab) are
   added to `HardwareInit::update()` so any engine-driven animation on them
   actually ticks (latent gap from the aux-light work).
5. **Docs** — EasyKit README pattern section + `GUIDE/HARDWARE_CONFIG.md` §4.5
   describe the group and its patterns.

## Non-goals (explicitly deferred)

- **Generic `pattern`/`mode` key in the hardware config schema.** The ditch
  config keeps its current shape; a second vehicle that needs a beacon/chase
  will drive the schema key design. Avoiding a config pattern language nobody
  has used yet.
- Rewriting the existing per-LED blink engine (SIMPLE/BURST/HEARTBEAT/CANDLE/
  MORSE stay as-is; the group is a separate, coexisting abstraction).
- Chase/beacon **hardware support in any shipped config** — the group API is
  the deliverable; only ditch uses it now.
