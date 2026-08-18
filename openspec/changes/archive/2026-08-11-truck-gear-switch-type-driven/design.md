## Context

The RadioKit UI has two pages (Truck / Loco) whose widget sets are mutually exclusive per vehicle model, and `/vehicle-config.json` declares the device type (`TRUCK`, `LOCOMOTIVE`, `EXCAVATOR`, `CUSTOM`) — the single source of truth. `VehicleController` currently branches on `RadioKit.getActivePage()`, which is the wrong axis: the firmware should branch on `profile.config.type`. The Truck page's `gear_switch` (an `RK_MultipleButton` radio group — value is the selected *index*, not a bitmask) is unwired; it should implement D/P/R. The manual indicator buttons were changed in the app to latched toggles, so the existing merge logic (`button.state || auto`) becomes a true latch/release control.

## Goals / Non-Goals

**Goals:**
- All widget reads driven by vehicle type, not active page.
- Truck gear switch D/P/R: Drive, Park (motor lock + parking-brake sound), Reverse (motor + beep + reversing light), shift sound on change while RUNNING.
- Manual indicator toggles merged with auto signals (latch/release semantics).
- Loco behavior (dir_switch direction) unchanged.

**Non-Goals:**
- No config schema or board changes.
- No changes to the brake blend, battery cutoff, engine state machine logic itself (only which widgets feed them).
- No new sound assets.

## Decisions

### D1. Type-driven widget selection
Compute once per update: `bool isLoco = (s_profile->config.type == RcEngineSound::VEHICLE_LOCOMOTIVE);` and use it for every widget read — throttle (`throttle_slider` vs `gas_pedal`), lights bitmask (`loco_light` vs `truck_light`), engine toggle (`engine_button` vs `start_button`), horn/bell, and aux_slider (Truck only). The `page` variable is no longer used for input selection.

- *Alternatives considered*: keep active-page branching — rejected: the config type is the declared single source of truth and the pages are only UI surfaces.

### D2. Gear decode
`gear_switch.rk.value` is the radio index: D=0, P=1, R=2. Add `s_gearPrev` static; a change in the index (0..2) is a gear change. If the value is out of range (3–255, e.g. initial state), treat as P (safest — no motion).

- *Alternatives considered*: bitmask decode — rejected: `RK_MultipleButton` is single-select and sends an index per the RadioKit value semantics.

### D3. Park behavior
When P: force `motorThrottle = 0` before the brake blend and motor output (so neither gas pedal nor brake blend can move it), and call `triggerParkingBrake(parkingActive)` with change detection. The engine power state is untouched (engine may stay RUNNING, idling in Park).

### D4. Reverse behavior
When R: `reverse = true` for trucks (mirroring `dir_switch` for locos), which negates the motor command, plays `triggerReversing`, and — via `applyLightsWithAutomation` — lights the reversing lamp automatically: `manualRev = (bits & 0x10) || reverse`.

### D5. Shift sound
`triggerShifting(true)` on a gear *change* (D/P/R index transition) only while `eState == RUNNING`. No continuous sound while holding a gear.

### D6. Manual indicator toggles
No new logic — the existing merge (`turnSignalL/R = button.state || auto`) already yields latch semantics once the buttons are `RK_ToggleButton` after re-import. The advanced-lighting spec delta documents the behavior.

## Risks / Trade-offs

- [Radio index vs app layout] The D=0/P=1/R=2 mapping depends on the item order in the app's gear_switch. → Verify after re-import; the decode is a single lookup if the order differs.
- [Park overriding inputs] Park is applied before the brake blend, so neither pedal can move the truck; battery cutoff still wins because it early-returns before all motor code.
- [Type gating regression] Any truck widget read left on the loco path (or vice versa) would misbehave. → The delta spec's "Truck widgets ignored on locomotive" scenario and the build review cover this.
- [Re-import dependency] The toggles/D-P-R labels must be in the app design. → Re-import + patch script before firmware validation.

## Migration Plan

1. Re-import `docs/radiokit-rc-ui-design.json` + `src/RADIOKIT.h` from the app (`/api/designs/1785927365527/{json,header}`), run `scripts/patch_radiokit_header.py`.
2. Firmware changes are compile-time; no config or flash migration.

## Open Questions

None — all semantics settled with the user (D=0/P=1/R=2, park lock + sound, reverse + beep + light, shift sound only while running, dir_switch for locos).
