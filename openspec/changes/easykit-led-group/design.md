# Design: EasyLEDGroup — coordinated multi-LED patterns

## The abstraction: a step sequencer

Every requested pattern (alternate, sync flash, chase, double strobe) is a
**timed sequence of steps**, where each step is a per-member duty vector plus
a duration:

```
pattern = [ step₀, step₁, … ]
step    = { uint8_t duty[memberCount], uint16_t durationMs }
```

The group advances one step at a time on a shared `millis()` timeline and
writes the step's duties to its member LEDs. Repeat flag loops the table.

```
┌──────────────────────────────────────────┐
│ EasyLEDGroup                            │
│  members: EasyLED*[N]                   │
│  pattern: step[] + repeat               │
│  update(): advance timeline, apply step │
│  startPattern(steps, count, repeat)     │
│  stop() → all members off               │
└──────────────────────────────────────────┘
```

## D1: Pattern as data, factories for ergonomics

The core API takes a raw step table. Built-in factory helpers build common
tables so callers never hand-write steps:

- `alternate(uint16_t intervalMs)` — 2+ members, counter-phase: `[100,0,…]`
  ↔ `[0,100,…]`, each `intervalMs`. (The ditch light: 8 ms ≈ 60 alternations/s.)
- `syncFlash(onMs, offMs)` — all members together.
- `chase(uint16_t intervalMs)` — one member lit at a time, moving down the
  list (beacon/light bar).
- `doubleStrobe(intervalMs, gapMs)` — two quick alternations then a pause
  (police double-flash).

Duties are percentages (0–100); the group resolves them against each member's
`getMaxDuty()` at apply time, matching `EasyLED::write(float percent)`.

## D2: Duty ownership & clean stop

While a pattern is running, the group **owns the duty of its members** — the
same rule the blink engine uses (no `setLight()` may target a running member).
`stop()` lands every member at 0 and marks the group idle. This gives battery
cutoff and hot-reload teardown a single `group.stop()` instead of relying on
callers passing zeroed bitmasks (as the current manual ditch code implicitly
does).

## D3: Separate from the per-LED blink engine

The group drives duty directly (step → `member.write(percent)`); it does not
reuse `_blinkTick`/`startBlink` state. Rationale: zero interaction risk with
turn-signal/hazard blinking, and the group's job (multi-LED timing) is
orthogonal to a single LED's pattern modes. Both coexist; the pump calls
`group.update()` alongside the per-LED `update()`s.

## D4: Ditch migration

- `HardwareInit` owns a `EasyLEDGroup s_ditchGroup` bound to `ditchLLed` /
  `ditchRLed`; `initLights()` starts `alternate(intervalMs)` when the config
  has ditch lights configured.
- The app toggle (loco light selector item F, bit 5) becomes: on →
  `s_ditchGroup.startPattern(...)` (or start-on-first-enable), off →
  `s_ditchGroup.stop()`. `VehicleController` drops `s_ditchLastToggle` and
  `s_ditchSide` entirely.
- Config shape **unchanged**: `left`/`right`/`brightness_max`/`interval_ms`.
  Edge-triggered like `setLightBlink`: repeated identical states are no-ops.

## D5: Pump completeness (latent gap fix)

`HardwareInit::update()` currently pumps only head/tail/brake/turnL/turnR/
reversing. The aux-light work added `ditchLLed`, `ditchRLed`, `stepLed`,
`cabLed` instances but **not** to the pump — so engine-driven animation on
them would never tick. This change adds them, plus `s_ditchGroup.update()`.

## D6: Config `pattern` key deferred

No generic `pattern`/`mode` key in the hardware config schema yet. The ditch
light keeps `interval_ms`. When a second vehicle needs a beacon/chase, that
change designs the schema surface (with the factories as the likely enum
values).

## Naming

`EasyLEDGroup` (class), factory methods on it. Member count 2–8; construction
takes an initializer list of `EasyLED*`.

## Risks / trade-offs

- [Vendored lib changes] → The repo already owns ESP32_EasyKit (committed in
  `lib/`, `HardwareInit` depends on its internals); the class is additive and
  backward compatible — existing `startBlink`/fade/breath callers unaffected.
- [Over-generalization] → Mitigated by deferring the config schema surface
  (D6); the sequencer itself is ~150 lines and the pattern abstraction is
  demonstrably shared by every flasher/beacon use case named.
- [Duty conflicts with blink engine] → D2 ownership rule + separate pump; the
  group's members are distinct LED objects from the turn-signal pair, so no
  pin overlap in any shipped config.
- [Flash cost] → One class + step tables in flash (est. < 1 KB); zero steady
  RAM beyond the member pointers and current step index; no per-loop cost
  beyond one `millis()` compare per member group.
