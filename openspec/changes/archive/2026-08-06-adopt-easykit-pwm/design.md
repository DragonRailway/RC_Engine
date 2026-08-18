# Design: Adopt ESP32_EasyKit as the PWM Layer

## Context

The project (`RC_brain`) is an ESP32-S3 RC vehicle controller. Today `platformio.ini` declares `ESP32_PWM_Fusion=symlink:///home/sun/Filelink/Arduino/libraries/ESP32_PWM_Fusion`, but that on-disk directory actually contains **ESP32_EasyKit v1.1.0** (EasyServo/EasyMotor/EasyLED over MCPWM + LEDC), a rewrite of the Dlloydev library by the project author. Analysis of the reference libraries in `references/` showed:

- `ESP32_PWM_Fusion/` (references copy) — pristine Dlloydev AnalogWrite, LEDC-only, but **nothing in the repo includes it**
- `ESP32Servo/`, `ESP32MCServo/`, `ESP32_MCPWM/` — all use the **legacy** `driver/mcpwm.h` API, which was removed in ESP-IDF 5.x (now `driver/deprecated/driver/mcpwm.h`); they cannot compile on this project's Arduino 3.x toolchain without patching
- EasyKit is the only library written against `driver/mcpwm_prelude.h` (the current API)

Meanwhile `common/HardwareInit.h` bypasses all libraries with raw `ledcAttach`/`ledcWrite`: motor at 24 kHz/8-bit, servo at 50 Hz/14-bit, lights at 5 kHz/10-bit. A fully-loaded config needs **9 LEDC channels but the ESP32-S3 only has 8** — a real exhaustion risk.

The author (user) developed EasyKit, so it is the most flexible option and can be extended as needed.

## Goals / Non-Goals

**Goals:**
- Vendor the dependency under its true identity: `lib/ESP32_EasyKit`
- Route all PWM output through EasyKit (motor, servo/ESC, lights) so hardware capacity is managed by its allocators
- Free LEDC channels by moving motor + servo to MCPWM (12 operator slots on S3)
- Keep the `HardwareInit` public API (`setMotor`/`setServo`/`setLight`) byte-for-byte compatible with callers
- Fix EasyKit bugs found during analysis (servo pulse scaling, inert `setFrequency`)

**Non-Goals:**
- No changes to config schema, `Config.h`, `PinMapper`, `VehicleController`, `src/main.cpp`, `src/RADIOKIT.h`, or `lib/SoundEngine`
- No changes to the sound engine's I2S audio path
- Not migrating/porting the other reference libraries (ESP32Servo, ESP32MCServo, ESP32_MCPWM) — they are documented as unbuildable instead
- No removal of `references/` content

## Decisions

### Decision 1: Adopt EasyKit as the single PWM layer
**Choice:** Use EasyKit classes in `HardwareInit` instead of raw LEDC calls and instead of any of the `references/` libraries.

**Rationale:** It is the only library that compiles on the project's Arduino 3.x / IDF 5.x toolchain (uses `mcpwm_prelude.h`), it already models exactly this board's split (MCPWM motor + servo, LEDC lights), it has thread-safe singleton allocators (LEDCManager, MCPWMManager) that solve the channel-budget problem, and it's authored by the project owner so it can be extended freely.

**Alternatives considered:**
- *Raw LEDC (status quo)*: works today but can exhaust 8 channels with a full config; no allocator protection; servo µs math is hand-rolled.
- *ESP32Servo (madhephaestus)*: dual LEDC+MCPWM architecture is attractive, but the vendored copy uses the legacy MCPWM API and fails to compile on this toolchain.
- *ESP32MCServo / ESP32_MCPWM*: legacy API, explicitly pending Arduino 3.x support / not a servo library.

### Decision 2: Fix EasyServo pulse scaling at the timer, not the writes
**Choice:** Set the EasyServo MCPWM timer `resolution_hz` to **1,000,000** (1 µs per tick) instead of the current 10 MHz, so µs-valued comparator writes become correct with no per-write scaling.

**Rationale:** Current code configures `resolution_hz = 10,000,000` (0.1 µs/tick) but writes `_currentUs` (e.g., 1500) directly to the comparator — producing a 150 µs pulse instead of 1500 µs. Keeping the comparator domain in microseconds (resolution 1 MHz) preserves every µs-based call site (`write`, `writeMicroseconds`, `_applyDuty`, `attach`) with zero other changes and still gives 1 µs pulse precision, well above the 0.5 µs servo resolution needed.

**Alternatives considered:**
- *Scale comparator values by 10×*: also correct, but touches every write site and keeps a footgun for future edits.

### Decision 3: Vendor EasyKit under `lib/` instead of symlinking
**Choice:** Copy the EasyKit source into the repo at `lib/ESP32_EasyKit` and reference it as a local library (mirroring the existing `lib/SoundEngine` pattern), removing the `ESP32_PWM_Fusion=symlink://...` entry from `platformio.ini`. The machine-local copy at `/home/sun/Filelink/Arduino/libraries/` remains the author's dev checkout.

**Rationale:** Names should be true and the repo should build anywhere. Symlinking to a machine-local path breaks builds on any machine that hasn't cloned that directory and hides in-repo fixes from version control. Vendoring means the servo pulse-scaling fix (Decision 2) and the `setFrequency` fix ship with the repo and builds are reproducible. The obsolete `references/ESP32_PWM_Fusion` (pristine Dlloydev) copy is deleted — nothing includes it and it cannot build on this toolchain. The accepted cost is source duplication between the dev checkout and `lib/`.

**Alternatives considered:**
- *Symlink to the renamed on-disk directory*: keeps a single source of truth but couples builds to a machine-local path and hides repo-specific fixes.

### Decision 4: Keep EasyKit objects as `HardwareInit` statics
**Choice:** `HardwareInit` owns one `EasyMotor` (re-used across HBRIDGE_A/B/ESC variants by re-`begin()`), one `EasyServo` for steering (plus ESC PPM), and up to six `EasyLED` instances, all as static members.

**Rationale:** The existing code is header-only with static members; this keeps that pattern, confines the change to `HardwareInit.h`, and lets `stopAll()`/`hotReload()` call `end()`/`detach()` then re-`begin()`.

### Decision 5: ESC via EasyServo, not EasyMotor
**Choice:** ESCs receive PPM via `EasyServo` (`minUs=1000, maxUs=2000, freq=50`) rather than `EasyMotor(DRIVER_1PWM)`.

**Rationale:** `DRIVER_1PWM` is duty-based unidirectional PWM, not a 1–2 ms PPM servo pulse. ESC semantics are identical to a servo channel; reusing EasyServo is the correct mapping.

## Risks / Trade-offs

- [EasyKit is a single-author (user-owned) hobby library with limited external review] → Mitigation: the author owns it; fixes ship directly in the local copy; changes are small and isolated to `HardwareInit`.
- [`LEDCManager` allocation does not model S3 timer-pair frequency sharing; bookkeeping may diverge from `ledcAttach` internals] → Mitigation: after wiring 6 lights, stress-test all-light configurations and verify actual frequencies on a scope/log; if divergences appear, fix the manager or pin explicit channels in `LEDConfig`.
- [MCPWM timer count (2 units × 3 timers, 6 operators/unit) limits simultaneous servos] → Mitigation: current needs (1 motor + 1–2 servos) are far below 12 slots; `MCPWMManager::allocate` returns `ERR_NO_FREE_OPERATOR` cleanly if ever exhausted.
- [Hot-reload re-`begin()` after `end()` may leave MCPWM timers enabled if teardown is incomplete] → Mitigation: verify with a hot-reload loop test; EasyKit's `end()` path releases timer/operator handles via `mcpwm_del_*` and the slot manager.
- [Vendoring duplicates EasyKit source between `lib/ESP32_EasyKit` and the author's dev checkout] → Mitigation: `lib/ESP32_EasyKit` is the source of truth for the project; fixes land there first and are synced back to the dev checkout periodically; the dev checkout is not required to build.

## Migration Plan

1. Vendor EasyKit into the repo: copy `/home/sun/Filelink/Arduino/libraries/ESP32_PWM_Fusion` → `lib/ESP32_EasyKit` (src/ + metadata + LICENSE; exclude build artifacts and .git).
2. Update `platformio.ini`: remove the `ESP32_PWM_Fusion=symlink://...` entry; reference `lib/ESP32_EasyKit` via a local lib_deps entry and/or `-I lib/ESP32_EasyKit/src` (mirroring `lib/SoundEngine`).
3. Update `openspec/config.yaml` context (and AGENTS.md if it mentions the old name).
4. Implement the `HardwareInit` swap: EasyMotor / EasyServo / EasyLED statics + `setMotor`/`setServo`/`setLight`/`stopAll` rewrite.
5. Fix EasyServo timer resolution (Decision 2) and `setFrequency` timer reconfiguration in `lib/ESP32_EasyKit/src/EasyServo.cpp`.
6. Delete the obsolete `references/ESP32_PWM_Fusion` (pristine Dlloydev) copy via `git rm -r`.
7. Add `references/README.md` documenting buildability of the remaining libraries on this toolchain.
8. Build `pio run -e TRACKLINK_V3` and `pio run -e MIKRO_V2`; smoke-test motor/servo/lights and hot-reload.

**Rollback:** Revert `platformio.ini` to the old `ESP32_PWM_Fusion=symlink://...` entry and restore `HardwareInit.h`; delete `lib/ESP32_EasyKit`. The change is confined to `platformio.ini`, `HardwareInit.h`, and the vendored library, so rollback is trivial.

## Open Questions

**Resolved:** `references/ESP32_PWM_Fusion` (pristine Dlloydev) will be **deleted** — it is obsolete, unbuildable on this toolchain, and unreferenced by the build.
