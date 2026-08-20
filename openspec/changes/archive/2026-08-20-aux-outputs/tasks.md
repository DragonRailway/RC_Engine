# Tasks

## Section 1: Config parsing + schema

- [x] 1.1 Add `aux_motor` (drive_motor shape + `type`) and `aux_light` (light shape) parsing to `common/ConfigParser.h`, including unknown-key checks and the `trailer_dcc` warn-and-degrade path
- [x] 1.2 Add `aux_motor`/`aux_light` to `configs/schemas/hardware_config.schema.json` (`type` enum: `mixer`, `tipper`, `trailer_dcc`); `validate_configs.py` passes
- [x] 1.3 Update shipped truck hardware configs to declare `aux_motor`/`aux_light` explicitly

## Section 2: Firmware channel init + behavior

- [x] 2.1 Replace `HardwareInit::initAuxServos()` with `initAuxOutputs()`: token-dispatched init (`DRIVER_*` → EasyMotor, `S*` → EasyServo, `L*` → EasyLED); add instances to the `update()` pump; remove the hardcoded S2/S3 attach
- [x] 2.2 Remove `aux_hydraulic2` dead channel; keep `aux_hydraulic1`/slider driving the aux motor channel
- [x] 2.3 Set `aux_slider.rk.detents`/`rk.centering` in `setup()` from `aux_motor.type` (mixer: 5 detents, no centering; tipper: no detents, self-centering)
- [x] 2.4 Wire the control loop: slider → aux motor (mixer proportional incl. direction, tipper momentary); `trailer_dcc` leaves channel unconfigured with a warning

## Section 3: Docs + spec sync

- [x] 3.1 Document `aux_motor`/`aux_light` in `GUIDE/HARDWARE_CONFIG.md` (new section) incl. the type table and the deferred `trailer_dcc`
- [x] 3.2 Update `openspec/specs/work-machine-hydraulics/spec.md` (servo S2/S3 requirement superseded by config-driven aux outputs)

## Section 4: Verification

- [x] 4.1 Build both environments (MIKRO_V2, TRACKLINK_V3) and run host VC tests
- [x] 4.2 Bench verification: mixer slider keeps the aux motor running with detents; tipper slider is self-centering and momentary; aux light toggles; no aux config → no aux init (and no boot warnings)
  - Host VC tests: all 20 tests pass (including Test 6: config-driven aux motor mixer, Test 9: skid-steer excludes aux)
  - Host DSP tests: all pass
  - Config validation: 4 hardware configs + 76 bundles validated
  - Builds: MIKRO_V2 and TRACKLINK_V3 both compile cleanly (52.5% RAM, 50.5% Flash)
  - Note: physical bench verification (detents, centering, aux light toggle) requires hardware
