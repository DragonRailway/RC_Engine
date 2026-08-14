# RC Brain — Hardware Config Reference

This document provides a description of the configuration options available
for the **hardware config** (`hardware-<BOARD>.json`). It is modeled after the
[Klipper configuration reference](https://www.klipper3d.org/Config_Reference.html).

The authoritative sources for this document are `common/Config.h`
(struct defaults), `common/ConfigParser.h` (parsed keys),
`common/PinMapper.h` and `boards/*.h` (pin vocabulary), and the shipped
configs in `configs/hardware_configs/`.

## Table of Contents

1. [Introduction](#1-introduction)
2. [sound](#2-sound)
3. [drivetrain](#3-drivetrain)
   - [drivetrain.drive_motor / left_motor / right_motor](#31-drivetraindrive_motor--left_motor--right_motor)
   - [drivetrain.steering_servo](#32-drivetrainsteering_servo)
   - [drivetrain.steering_sensitivity](#33-drivetrainsteering_sensitivity)
4. [lights](#4-lights)
   - [lights.head_light / tail_light](#41-lightshead_light--tail_light)
   - [lights.brake_light](#42-lightsbrake_light)
   - [lights.turn_light](#43-lightsturn_light)
   - [lights.reversing_light](#44-lightsreversing_light)
5. [animation](#5-animation)
6. [telemetry](#6-telemetry)
7. [battery](#7-battery)
8. [Pin Reference](#8-pin-reference)
   - [MIKRO_V2 pins](#81-mikro_v2-pins)
   - [TRACKLINK_V3 pins](#82-tracklink_v3-pins)
   - [Hardware token vocabulary](#83-hardware-token-vocabulary)
9. [Example](#9-example)

---

## 1. Introduction

The hardware config describes **what is physically wired to the board**:
motor(s), steering servo, lights, and how to interpret power/telemetry. It is
loaded once at boot from `/hardware-config.json` on the device LittleFS.

In the repo it lives at `configs/hardware_configs/hardware-<BOARD>.json` and
is flashed alongside the vehicle bundle:

```bash
python3 scripts/build_fs.py --board MIKRO_V2 --vehicle ScaniaV8
```

The board is selected at compile time (`-D MIKRO_V2` or `-D TRACKLINK_V3`);
the same config keys mean the same thing on both boards, but the `hardware`
**tokens resolve to different physical GPIOs per board** — see
[§8 Pin Reference](#8-pin-reference).

### How a config is read

- **Defaults everywhere.** Every parameter has a firmware default. A missing
  section, or a missing key within a section, silently falls back to the
  default listed in this document. A config can be very minimal.
- **Keys are case-insensitive with legacy fallbacks.** The parser accepts
  both `snake_case` keys and their legacy `UPPER_CASE` variants
  (e.g. `sound` or `SOUND`). Prefer `snake_case` for new configs.
- **No schema validation.** Unknown keys are **silently ignored**. An
  unrecognized `hardware` token resolves to "not configured" — the affected
  light or motor simply never turns on. **A typo looks like a dead output,
  not an error.** This guide is the de-facto schema: check names here.
- **Quirks are documented.** Where a key is accepted but ignored, or a value
  is forced by the firmware, it is called out explicitly in this document.

---

## 2. sound

Master audio volume for the sound engine.

The following parameters are available in the `sound` section:

- `volume`: master volume of the sound engine, in percent.
  **Type:** integer · **Default:** `80` · **Range:** 0–100.

  This feeds the sound engine's master volume. Per-slot levels (start, idle,
  rev, horn, …) are configured separately in the **vehicle** config
  (`sound_volumes`), not here.

Example:

```json
"sound": {
    "volume": 80
}
```

---

## 3. drivetrain

Describes how the vehicle is driven. The parser detects the drivetrain type
**by which keys are present**:

| Keys present | Drivetrain type | Parameters honored |
|---|---|---|
| `left_motor` (and `right_motor`) | Skid-steer | `left_motor`, `right_motor`, `steering_sensitivity` |
| otherwise (default) | Ackermann | `drive_motor`, `steering_servo` |

> ⚠️ **The fork.** A `left_motor` key anywhere in `drivetrain` switches the
> whole section to skid-steer mode — `drive_motor` and `steering_servo` are
> then **ignored**. Don't mix the two layouts.
>
> Legacy: a top-level `DRIVE_MOTOR` (outside `drivetrain`) and a top-level
> `STEERING_SERVO` are still parsed as Ackermann.

### 3.1 drivetrain.drive_motor / left_motor / right_motor

Motor driving the wheels. `drive_motor` is used for Ackermann layouts;
`left_motor` and `right_motor` for skid-steer (one per track side). All three
share the same parameter block.

The following parameters are available in the `drivetrain.drive_motor`
(or `left_motor` / `right_motor`) section:

- `hardware`: the hardware the motor is attached to. **Required** for the
  motor to be configured. Allowed values:
  - `HBRIDGE_A`, `HBRIDGE_B` — on-board H-bridge drivers (see
    [§8 Pin Reference](#8-pin-reference) for pin assignments)
  - `S1` … `S4` — an ESC on a servo pin
- `type`: motor type. **Type:** string · **Default:** derived from
  `hardware`.

  > ⚠️ **Accepted but ignored.** The parser derives the type from
  > `hardware` — `HBRIDGE_*` ⇒ `hbridge`, `S*` ⇒ `esc`. The `type` key
  > in the shipped configs (`"type": "hbridge"`) is documentation only.

- `frequency`: PWM frequency in Hz. **Type:** integer · **Default:** `20000`.
- `direction`: motor polarity behavior. **Type:** string · **Default:**
  `forward`. Allowed values:
  - `forward` — command speed applied as-is
  - `reverse` — command speed negated (motor wired backwards)
  - `uni_forward` — always forward (`abs(speed)`); ignores reverse commands
  - `uni_reverse` — always reverse; ignores forward commands
- `duty.min`: minimum motor duty (percent) applied at any non-zero speed —
  the "dead zone" floor. **Type:** integer · **Default:** `20` · **Range:** 0–100.
- `duty.max`: maximum motor duty (percent) applied at full speed.
  **Type:** integer · **Default:** `90` · **Range:** 0–100.

Example (Ackermann drive motor):

```json
"drive_motor": {
    "hardware": "HBRIDGE_A",
    "frequency": 20000,
    "direction": "forward",
    "duty": {
        "min": 20,
        "max": 90
    }
}
```

### 3.2 drivetrain.steering_servo

Servo that steers the front wheels (Ackermann layouts).

The following parameters are available in the `drivetrain.steering_servo`
section:

- `hardware`: the servo pin. **Required** for steering to be configured.
  Allowed values: `S1` … `S4` (see [§8 Pin Reference](#8-pin-reference)).
- `frequency`: servo PWM frequency in Hz. **Type:** integer · **Default:** `50`.
- `endpoints.left`: pulse width (µs) at full left lock.
  **Type:** integer · **Default:** `1350`.
- `endpoints.right`: pulse width (µs) at full right lock.
  **Type:** integer · **Default:** `1650`.
- `endpoints.center`: pulse width (µs) at straight-ahead (center).
  **Type:** integer · **Default:** `1500`.

Example:

```json
"steering_servo": {
    "hardware": "S1",
    "frequency": 50,
    "endpoints": {
        "left": 1350,
        "right": 1650,
        "center": 1500
    }
}
```

### 3.3 drivetrain.steering_sensitivity

Differential steering sensitivity for **skid-steer** layouts (how much the
left/right motor speeds diverge for a given steering command).

- `steering_sensitivity`: **Type:** integer · **Default:** `80` · **Range:** 0–100.

  > Only read in skid-steer mode; ignored for Ackermann layouts.

---

## 4. lights

On-board LED outputs. All `hardware` values are `L0` … `L8` tokens (the
available range depends on the board — see [§8 Pin Reference](#8-pin-reference)).

> **Unconfigured lights.** A light whose `hardware` token doesn't resolve is
> simply never driven — no error is raised. Verify the token against your
> board's pin table.

### 4.1 lights.head_light / tail_light

- `hardware`: the LED pin. **Required** for the light to be configured.
- `brightness_max`: maximum brightness in percent.
  **Type:** integer · **Default:** `60` · **Range:** 0–100.

Example:

```json
"head_light": {
    "hardware": "L1",
    "brightness_max": 60
}
```

### 4.2 lights.brake_light

- `hardware`: the LED pin. **Required** for the light to be configured.

> ⚠️ **`brightness_max` is ignored here.** The firmware forces the brake
> light to 100% brightness. A `brightness_max` key is accepted but has no
> effect.

Example:

```json
"brake_light": {
    "hardware": "L3"
}
```

### 4.3 lights.turn_light

Direction indicators, with one LED per side.

The following parameters are available in the `lights.turn_light` section:

- `left.hardware`: LED pin for the left indicator. **Required** for that
  side to blink.
- `right.hardware`: LED pin for the right indicator. **Required** for that
  side to blink.
- `brightness_max`: maximum brightness in percent.
  **Type:** integer · **Default:** `60` · **Range:** 0–100.
- `interval_on`: blink on-duration in ms. **Type:** integer · **Default:** `500`.
- `interval_off`: blink off-duration in ms. **Type:** integer · **Default:** `500`.
- `type`: **Type:** string · **Default:** none.

  > ⚠️ **Accepted but ignored.** The shipped configs include `"type": "blink"`,
  > but the parser does not read this key. Indicator behavior is always a
  > blink driven purely by `interval_on` / `interval_off`. The key is
  > documented here so it isn't mistaken for a feature toggle.

Example:

```json
"turn_light": {
    "left":  { "hardware": "L4" },
    "right": { "hardware": "L5" },
    "brightness_max": 60,
    "interval_on": 500,
    "interval_off": 500
}
```

### 4.4 lights.reversing_light

Light driven while reversing.

- `hardware`: **Type:** string — either a **pin token** (`L0` … `L8`) or a
  **light alias** naming another configured light:
  - `"head_light"` — reuse the head light's pin
  - `"tail_light"` — reuse the tail light's pin
  - `"brake_light"` — reuse the brake light's pin

  If the alias refers to a light that isn't configured, the reversing light
  stays unconfigured.

> ⚠️ **Brightness forced to 100.** Like the brake light, the reversing light
> is always full brightness; `brightness_max` is not honored.

Example (mirroring the brake light):

```json
"reversing_light": {
    "hardware": "brake_light"
}
```

---

## 5. animation

Tuning for the EasyKit easing/fade/blink animation engines. Global defaults —
absent from a hardware config, these values apply as-is.

The following parameters are available in the `animation` section:

- `easing_speed_deg_s`: auxiliary-servo movement speed in degrees/second.
  **Type:** float · **Default:** `180` · `0` = instant (no easing).
- `easing_k_in`: easing strength at move start.
  **Type:** float · **Default:** `0.2` · **Range:** 0–1.
- `easing_k_out`: easing strength at move end.
  **Type:** float · **Default:** `0.8` · **Range:** 0–1.
- `fade_duration_ms`: headlight fade transition time in ms.
  **Type:** integer · **Default:** `250`.

Example:

```json
"animation": {
    "easing_speed_deg_s": 180,
    "easing_k_in": 0.2,
    "easing_k_out": 0.8,
    "fade_duration_ms": 250
}
```

---

## 6. telemetry

Calibration of the battery voltage sense. The values map raw ADC readings to
real volts.

The following parameters are available in the `telemetry` section:

- `voltage_scale`: scale factor applied to the voltage sense reading.
  **Type:** float · **Default:** compile-time `VSCALE` (`1.8` for both
  shipped envs in `platformio.ini`).
- `voltage_offset`: offset added after scaling (volts).
  **Type:** float · **Default:** compile-time `VOFFSET` (`-0.2` for both
  shipped envs).

  When present, these values override the compile-time `VSCALE`/`VOFFSET`
  macros — useful for calibrating a specific board without rebuilding.

Example:

```json
"telemetry": {
    "voltage_scale": 1.8,
    "voltage_offset": -0.2
}
```

---

## 7. battery

LiPo pack configuration. All voltages are **per cell**.

The following parameters are available in the `battery` section:

- `cell_count`: number of cells in the pack. **Type:** integer · **Default:**
  `0` (auto-detect) · **Range:** 0–4.
  - `0` — voltage-based auto-detection at boot (legacy behavior)
  - `1`–`4` — fixed cell count; the single source of truth for the pack
- `cutoff_voltage`: low-voltage cutoff **per cell** (V).
  **Type:** float · **Default:** `3.3`.
- `full_voltage`: fully-charged voltage **per cell** (V).
  **Type:** float · **Default:** `4.2`.

Example:

```json
"battery": {
    "cell_count": 1,
    "cutoff_voltage": 3.4,
    "full_voltage": 4.2
}
```

---

## 8. Pin Reference

The `hardware` values in a config are **logical tokens**, not GPIO numbers.
Each token resolves to a different physical pin depending on the board
selected at compile time. **Always check your board's table.**

### 8.1 MIKRO_V2 pins

| Token | GPIO | Notes |
|---|---|---|
| `L0` | 36 | LED channel |
| `L1` | 38 | LED channel |
| `L2` | 39 | LED channel |
| `L3` | 40 | LED channel |
| `L4` | 41 | LED channel |
| `L5` | 42 | LED channel |
| `L6` | 43 | LED channel |
| `L7` | 1  | LED channel |
| `L8` | 2  | LED channel |
| `S1` | 5  | Servo / ESC |
| `S2` | 6  | Servo / ESC |
| `S3` | 7  | Servo / ESC |
| `S4` | 8  | Servo / ESC |
| `HBRIDGE_A` | — | PWM1=18, PWM2=21, EN=17, BEMF=9 · dual-PWM bridge |
| `HBRIDGE_B` | — | PWM1=12, PWM2=13, EN=11, BEMF=10 · dual-PWM bridge |

### 8.2 TRACKLINK_V3 pins

| Token | GPIO | Notes |
|---|---|---|
| `L0` | 42 | **Built-in LED** |
| `L1` | 6  | LED channel |
| `L2` | 7  | LED channel |
| `L3` | 8  | LED channel |
| `L4` | 9  | LED channel |
| `L5` | 10 | LED channel |
| `L6` | 11 | LED channel |
| `S1` | 1  | Servo / ESC |
| `S2` | 2  | Servo / ESC |
| `HBRIDGE_A` | — | PWM1=13, PWM2=14, BEMF=4, common EN=12 · dual-PWM bridge |
| `HBRIDGE_B` | — | DIR=15, PWM=16, BEMF=5, common EN=12 · DIR+PWM bridge |

Note the per-board differences — e.g. `L1` is **GPIO 38** on MIKRO_V2 but
**GPIO 6** on TRACKLINK_V3, and `S2` is available on TRACKLINK_V3 only up to
`S2` (MIKRO_V2 goes to `S4`).

### 8.3 Hardware token vocabulary

- `L<n>` — LED channels, `L0`–`L8` (board-dependent upper bound).
- `S<n>` — servo/ESC channels, `S1`–`S4` (board-dependent upper bound).
- `HBRIDGE_A`, `HBRIDGE_B` — **semantic markers** for the on-board H-bridge
  drivers, resolved per board to their PWM/DIR/EN/BEMF pins (tables above).
  `HBRIDGE_A` is a dual-PWM bridge on both boards; `HBRIDGE_B` is dual-PWM on
  MIKRO_V2 but DIR+PWM on TRACKLINK_V3.
- Any other token (e.g. a typo) resolves to **not configured** — the output
  is simply never driven.

---

## 9. Example

A complete annotated hardware config (the shipped MIKRO_V2 config):

```json
{
    "sound": {
        "volume": 80
    },
    "drivetrain": {
        "drive_motor": {
            "type": "hbridge",              // documentation only — derived from hardware
            "hardware": "HBRIDGE_A",        // dual-PWM bridge: PWM1=18, PWM2=21, EN=17
            "frequency": 20000,             // PWM rate, Hz
            "direction": "forward",         // forward | reverse | uni_forward | uni_reverse
            "duty": {
                "min": 20,                  // dead-zone floor, %
                "max": 90                   // full-speed ceiling, %
            }
        },
        "steering_servo": {
            "hardware": "S1",               // GPIO 5 on MIKRO_V2
            "frequency": 50,
            "endpoints": {
                "left": 1350,               // µs
                "right": 1650,              // µs
                "center": 1500              // µs
            }
        }
    },
    "lights": {
        "head_light": {
            "hardware": "L1",               // GPIO 38 on MIKRO_V2
            "brightness_max": 60
        },
        "tail_light": {
            "hardware": "L2",               // GPIO 39
            "brightness_max": 60
        },
        "brake_light": {
            "hardware": "L3"                // brightness forced to 100
        },
        "turn_light": {
            "left":  { "hardware": "L4" },  // GPIO 41
            "right": { "hardware": "L5" },  // GPIO 42
            "brightness_max": 60,
            "type": "blink",                // accepted but ignored
            "interval_on": 500,             // ms
            "interval_off": 500             // ms
        },
        "reversing_light": {
            "hardware": "brake_light"       // light alias — mirrors brake light pin
        }
    },
    "animation": {
        "easing_speed_deg_s": 180,          // 0 = instant
        "easing_k_in": 0.2,
        "easing_k_out": 0.8,
        "fade_duration_ms": 250
    },
    "telemetry": {
        "voltage_scale": 1.8,               // overrides compile-time VSCALE
        "voltage_offset": -0.2              // overrides compile-time VOFFSET
    },
    "battery": {
        "cell_count": 1,                    // 0 = auto-detect
        "cutoff_voltage": 3.4,              // per cell, V
        "full_voltage": 4.2                 // per cell, V
    }
}
```
