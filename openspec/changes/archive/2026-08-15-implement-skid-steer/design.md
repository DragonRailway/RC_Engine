## Context

The config layer is already skid-steer aware: `ConfigParser` detects `left_motor`/`right_motor` and sets `drivetrainType = SKID_STEER`, and `VehicleController::update()` computes a proper differential (`left = throttle + steer·sens/100`, `right = throttle − steer·sens/100`, negated in reverse, zeroed in park, brake-blended before the mix). The gap is entirely in `HardwareInit`:

```cpp
// init(): SKID_STEER branch
initDriveMotor(hw.leftMotor);   // writes motorType/… + driveMotor.begin()
initDriveMotor(hw.rightMotor);  // OVERWRITES the same statics + re-begins driveMotor

// setSkidMotors()
static void setSkidMotors(int16_t leftSpeed, int16_t rightSpeed) { setMotor(leftSpeed); }
```

So only one physical output exists (`driveMotor`, configured by whichever of left/right was initialized last) and only `leftSpeed` is ever applied.

Board constraint: MIKRO_V2 and TRACKLINK_V3 each expose exactly **two** H-bridge outputs (`DRIVER_A`, `DRIVER_B`) plus servo pins (`S1`–`S4`). The aux-motor feature (mixer/tipper) currently claims `DRIVER_B` on the MIKRO_V2 truck config — that is an Ackermann config, so no conflict there; but a skid config needs the second output for the right track.

## Goals / Non-Goals

**Goals:**
- Skid-steer configs drive two independent motor channels with the differential mix already computed in `VehicleController`.
- Ackermann behavior byte-for-byte unchanged.
- Explicit, schema-validated `drivetrain.type` token with backward-compatible inference.
- Clear degradation + warnings for invalid skid configs (missing motor, aux conflict) — no silent misconfiguration.
- Host-verifiable via the existing host_vc harness.

**Non-Goals:**
- Torque vectoring / electronic differential tuning (sensitivity is a single scalar, as today).
- Supporting aux work-machine outputs on a skid-steer board (the second output is the right track; documented exclusion).
- Skid-steer on GTRACK/excavator (board has a single driver; excavator control surface is deferred anyway).
- Changing the differential math in `VehicleController` (already correct per spec).

## Decisions

### D1: Model each track as a motor *channel* in HardwareInit

Introduce a small `MotorChannel` struct (type, direction, duty min/max, attached flag, plus a pointer to its output object — `EasyMotor*` for H-bridge, `EasyServo*` for ESC/PPM). Extract the wiring logic from `initDriveMotor()` into `initChannel(MotorChannel&, const DriveMotor&)` and the command logic from `setMotor()` into `setChannel(MotorChannel&, int16_t speed)`.

- **Ackermann**: only the left channel is initialized, from `drive_motor` — it maps to the same `driveMotor`/`escServo` objects as today, so `setMotor()` becomes `setChannel(leftCh, speed)` and existing behavior is preserved exactly (same polarity, duty window, ESC pulse math, easing).
- **Skid**: left channel from `left_motor` (→ `driveMotor`/`escServo`), right channel from `right_motor` (→ `auxMotor`/`auxServo`, reusing the objects the aux feature already creates). `setSkidMotors(l, r)` calls `setChannel(leftCh, l)` + `setChannel(rightCh, r)`.

**Alternatives considered:** (a) a second dedicated `EasyMotor`/`EasyServo` object — rejected: more state to manage for no gain, since the aux objects are idle in skid mode by definition. (b) Keeping the current statics and duplicating them for the right side — rejected: setMotor's duty/ESC logic would be copy-pasted; the channel abstraction centralizes it.

### D2: Explicit `drivetrain.type` token with inference fallback

`ConfigParser` reads `drivetrain.type` (`"ackermann"` | `"skid_steer"`) first; when absent it uses the current key-presence inference (`left_motor` present ⇒ skid). An unrecognized value logs `WARN` and falls back to inference. The schema gains the enum so `build_fs.py`/`validate_configs.py` catch typos at deploy time.

**Why:** the project's config model is declarative (vehicle config never mentions pins), the schema uses `additionalProperties: false` so an undocumented key would be rejected, and `"presence of a key implies mode"` is fragile — e.g. the vehicle-control-loop spec scenario wording becomes "drivetrain is declared". Old configs keep working untouched.

### D3: Skid mode excludes aux_motor (documented + warned)

In skid mode the right track owns the second motor output, so `aux_motor` cannot be configured. The parser logs `WARN: aux_motor: ignored in skid-steer mode (right track owns the second motor output)` and leaves the aux channel unconfigured — the same degrade-with-warning pattern already used for `trailer_dcc`. No hard error (project convention: warn-and-continue).

**Alternative considered:** allow aux on an `S*` ESC alongside a driver-based right track — rejected: three motor channels on a two-driver board only works with ESC-on-servo right track, which is a niche nobody asked for; the exclusion is simpler and honest.

### D4: `build_fs.py --hardware <variant>`

The repo names configs descriptively (`hardware-MIKRO_V2-truck.json`) and `build_fs.py` picks a **unique** `hardware-<BOARD>-*.json` match. A second MIKRO_V2 config would break that rule, so add `--hardware skid` (default: current unique-match behavior) to select `hardware-<BOARD>-<variant>.json` and stage it as `/hardware-<BOARD>.json`.

### D5: Right-track polarity matches the per-board driver conventions

The right channel reuses the exact wiring logic already proven for the drive motor: `pins.dualPwm` selects `DRIVER_2PWM` vs `DRIVER_1PWM_1DIR` with the same invert flag (DRIVER_B is dual-PWM on MIKRO_V2 but DIR+PWM on TRACKLINK_V3). No new polarity logic — one code path, both channels.

## Risks / Trade-offs

- [A right-track with inverted polarity on some board/driver combo] → the host test asserts sign of the right channel for forward/reverse; the per-board pin tables in `GUIDE/HARDWARE_CONFIG.md` already call out the DRIVER_B difference; hardware verification (scripts/hardware_verification.py) is the final gate.
- [Schema regression for existing Ackermann configs] → `drivetrain.type` is optional in the schema; `validate_configs.py` runs over every config in CI; all three existing hardware configs must still validate.
- [Channel refactor accidentally changes Ackermann duty/pulse behavior] → `setChannel` is a straight extraction of today's `setMotor` math; host_vc Tests 1–6 (brake blend, reverse, aux) are the regression net.
- [Users with an in-the-field skid config rely on inference] → inference fallback preserved; nothing breaks.

## Migration Plan

No migration: the token is optional and defaults to today's behavior. Rollback is a revert of the HardwareInit/ConfigParser changes; configs are unaffected.

## Open Questions

None blocking. (Resolved during design: right track may be H-bridge `DRIVER_B` or ESC on `S*`, per `right_motor.hardware`; skid + aux coexistence is excluded by D3.)
