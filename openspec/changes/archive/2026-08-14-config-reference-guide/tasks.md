## 1. Guide scaffolding

- [x] 1.1 Create `GUIDE/README.md` index with links to the reference and a note that the vehicle-config reference is a future stub
- [x] 1.2 Create `GUIDE/Config_Reference.md` with the Klipper-style table of contents (anchor links to every section)

## 2. Section reference (Config_Reference.md)

- [x] 2.1 Write the introduction: file layout, loading at boot, defaults, no-validation caveat, case-insensitive legacy keys
- [x] 2.2 Document the `sound` section (`volume`)
- [x] 2.3 Document the `drivetrain` section: the Ackermann/Skid-steer fork, shared `drive_motor`/`left_motor`/`right_motor` parameter block, `steering_servo`, `steering_sensitivity`
- [x] 2.4 Document the `lights` section: `head_light`, `tail_light`, `brake_light`, `turn_light` (incl. accepted-but-ignored `type`), `reversing_light` (incl. light-alias values)
- [x] 2.5 Document the `animation` section (easing + fade parameters)
- [x] 2.6 Document the `telemetry` section (`voltage_scale`, `voltage_offset` and VSCALE/VOFFSET fallbacks)
- [x] 2.7 Document the `battery` section (`cell_count` 0=auto, `cutoff_voltage`, `full_voltage`)

## 3. Pin Reference + example

- [x] 3.1 Write the per-board pin reference tables (MIKRO_V2, TRACKLINK_V3): LED tokens, servo tokens, H-bridge assignments, semantic `HBRIDGE_*` markers
- [x] 3.2 Write the complete annotated example config (from the shipped MIKRO_V2 config)

## 4. Validation

- [x] 4.1 Cross-check every documented default/allowed value against `common/Config.h`, `common/ConfigParser.h`, `common/PinMapper.h`, `boards/*.h`, and the shipped configs
- [x] 4.2 Verify all TOC anchors resolve and markdown renders cleanly
