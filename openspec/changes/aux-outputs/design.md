# Design: aux_motor / aux_light outputs

## Config shapes (mirroring the main outputs)

```jsonc
// hardware-<BOARD>.json
"aux_motor": {
    "hardware": "DRIVER_B",        // S*  → servo/ESC output (PPM, like drive ESC)
                                   // DRIVER_* → motor-driver output (H-bridge)
    "frequency": 20000,
    "direction": "forward",
    "duty": { "min": 20, "max": 90 },
    "type": "mixer"                // mixer | tipper   (trailer_dcc deferred)
},
"aux_light": {
    "hardware": "L4",
    "brightness_max": 60
}
```

`aux_motor` reuses the `drive_motor` parse/init path wholesale; `aux_light`
reuses the light path. The `hardware` token decides servo-vs-driver, exactly
as it does for `drive_motor` (DRIVER_* ⇒ H-bridge, S* ⇒ ESC/PPM) — no separate
`aux_servo` key needed.

## D1: The `type` field is purpose, not electrical kind

`type: mixer | tipper` selects the *drive behavior* and the app control
profile. Electrical kind (servo/driver) is derived from the `hardware` token —
same rule as `drive_motor` (where `type` was removed for being redundant).
The name `type` is kept per explicit choice; it means "aux purpose".

## D2: Mixer vs tipper behavior (both on aux_slider)

| | mixer | tipper |
|---|---|---|
| slider detents | 5 (snap positions) | 0 (continuous) |
| slider centering | none (`RK_SPRING_NONE`) | self-centering (`RK_SPRING_CENTER`) |
| motor drive | proportional to position; keeps running | momentary — follows position while held |
| range | -100..100 (direction via sign) | -100..100 |

The firmware sets `aux_slider.rk.detents` / `rk.centering` in `setup()`
(after config load, before connect) based on `aux_motor.type` —
`RK_Slider` exposes both as public runtime fields (verified in
`lib/rk-arduino/src/widgets/Slider.h`). The control loop then drives the aux
channel from the slider value.

## D3: No legacy aux outputs

`HardwareInit::initAuxServos()` (hardcoded S2/S3 auto-attach) is removed.
Aux outputs exist only when declared — "all servo outputs can be configured
for any use". Existing truck hardware configs gain `aux_motor`/`aux_light`
keys explicitly. `aux_hydraulic2` (never driven by any widget) is removed
with the old servo path.

## D4: Channel init by token

One `initAuxOutputs()` switches on the resolved token family:
- `DRIVER_*` → `EasyMotor` H-bridge (same begin logic as drive motor)
- `S*` → `EasyServo` PPM (same begin logic as steering/ESC)
- `L*` → `EasyLED` (for `aux_light`)
All instances join the `HardwareInit::update()` pump.

## D5: Vehicle config stays universal

`vehicle.json` carries no pins or channels — aux behavior (sounds, features)
is keyed off aux activity generically, as today (`triggerHydraulicFlow` on
`abs(slider) > 10`; `dump_bed_enabled` remains a vehicle feature flag). The
hardware config is the sole owner of "what is wired and what it runs".

## D6: Deferred surfaces

- `trailer_dcc` — reserved in the schema enum; firmware warns
  "aux_motor type 'trailer_dcc' not yet implemented" and leaves the channel
  unconfigured until a later change adds the protocol.
- Excavator profiles, main-output migration — out of scope.

## Risks / trade-offs

- [Hardware configs must be updated or lose aux] → D3 removes the fallback
  deliberately; the shipped `-truck` configs are updated in this change.
- [Slider config timing] → Widget config must be set before the app connects;
  `setup()` after config load satisfies this (same flow as today's hardcoded
  values).
- [Servo vs driver ambiguity] → Token-derived, identical to `drive_motor`;
  no new ambiguity.
- [Flash/RAM] → One aux motor + one aux light instance: negligible.
