# RC Engine — Hardware Config Reference

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
   - [drivetrain.drive_motor / left_motor / right_motor](#31-drivetraindrive_motor-left_motor-right_motor)
   - [drivetrain.steering_servo](#32-drivetrainsteering_servo)
   - [drivetrain.steering_sensitivity](#33-drivetrainsteering_sensitivity)
4. [lights](#4-lights)
   - [lights.head_light / tail_light](#41-lightshead_light-tail_light)
   - [lights.brake_light](#42-lightsbrake_light)
   - [lights.turn_light](#43-lightsturn_light)
   - [lights.reversing_light](#44-lightsreversing_light)
   - [lights.ditch_light / step_light / cab_light](#45-lightsditch_light-step_light-cab_light)
5. [aux_motor / aux_light](#5-aux_motor--aux_light)
   - [aux_motor](#51-aux_motor)
   - [aux_motor.type (mixer / tipper)](#52-aux_motortype-mixer--tipper)
   - [aux_light](#53-aux_light)
6. [animation](#6-animation)
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
- **Semantic validation warns (does not halt).** Since the
  `config-schema-validation` change, the firmware logs a `WARN:` line for
  every config mistake it finds — unknown keys, unrecognized `hardware`
  tokens, out-of-range values, and unrecognized enum strings — at boot and
  on hot-reload. The config still loads (degraded-but-running, as before),
  but now the mistake is visible instead of a silently-dead output.
- **Flash-time schema check.** `scripts/build_fs.py` validates the hardware
  config against `configs/schemas/hardware_config.schema.json` before
  staging — violations abort the flash with the exact paths listed.
  `scripts/validate_configs.py` runs the same checks across every config in
  the repo (CI-able).
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

Describes how the vehicle is driven. The drivetrain type is declared with
`drivetrain.type`; when the token is absent it is inferred **by which keys are
present** (legacy behavior):

| `drivetrain.type` | Parameters honored |
|---|---|
| `"skid_steer"` | `left_motor`, `right_motor`, `steering_sensitivity` |
| `"ackermann"` (default) | `drive_motor`, `steering_servo` |

- `type`: the drivetrain layout. **Type:** string · **Default:** inferred from
  key presence (`left_motor` present ⇒ `"skid_steer"`, otherwise `"ackermann"`).
  Allowed values: `"ackermann"`, `"skid_steer"`. An unrecognized value logs a
  boot `WARN` and falls back to the key-presence inference.

> ⚠️ **The fork.** `"skid_steer"` uses `left_motor` + `right_motor` —
> `drive_motor` and `steering_servo` are then **ignored**. `"ackermann"` uses
> the opposite layout. Don't mix the two.
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
  - `DRIVER_A`, `DRIVER_B` — on-board motor drivers (see
    [§8 Pin Reference](#8-pin-reference) for pin assignments)
  - `S1` … `S4` — an ESC on a servo pin
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
    "hardware": "DRIVER_A",
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

### 3.4 Example: skid-steer layout

Two tracks, each on its own motor driver (MIKRO_V2: `DRIVER_A` + `DRIVER_B`):

```json
"drivetrain": {
    "type": "skid_steer",
    "left_motor": {
        "hardware": "DRIVER_A",
        "frequency": 20000,
        "direction": "forward",
        "duty": { "min": 20, "max": 90 }
    },
    "right_motor": {
        "hardware": "DRIVER_B",
        "frequency": 20000,
        "direction": "forward",
        "duty": { "min": 20, "max": 90 }
    },
    "steering_sensitivity": 80
}
```

The left track runs on the drive output (`DRIVER_A` or an `S*` ESC), the right
track on the second motor output (`DRIVER_B` or another `S*` ESC). The `right`
track's polarity, duty window, and electrical kind are configured independently
of the left track, per [`left_motor`/`right_motor`](#31-drivetraindrive_motor--left_motor--right_motor).

> ⚠️ **Aux exclusion.** In skid-steer mode the second motor output is the right
> track, so `aux_motor` cannot be configured — the firmware logs
> `WARN: aux_motor: ignored in skid-steer mode` and the aux channel stays
> unconfigured. A missing `left_motor`/`right_motor` on a skid config logs a
> `WARN` and that track stays unconfigured.

---

## 4. lights

On-board LED outputs. All `hardware` values are `L0` … `L8` tokens (the
available range depends on the board — see [§8 Pin Reference](#8-pin-reference)).

> **Unconfigured lights.** A light whose `hardware` token doesn't resolve is
> simply never driven — no error is raised. Verify the token against your
> board's pin table.

### Lighting Bitmask Reference (8-Bit Control Mapping)

The RadioKit control surface transmits lighting states via an 8-bit selection mask (`truck_light` for Road Vehicles / Trucks, `loco_light` for Locomotives). The firmware decodes these bits as follows:

| Bit | Hex / Val | Road Vehicle / Truck (`truck_light`) | Locomotive (`loco_light`) | Automation & Mixing |
| :---: | :---: | :--- | :--- | :--- |
| **0** | `0x01` (1) | **Head Light** (`lights.head_light`) | **Headlight** (`lights.head_light`) | Truck fades `head_light` to 40%; Loco runs at 100%. `tail_light` automatically glows at 30% when active. |
| **1** | `0x02` (2) | **High Beam** (`lights.full_beam`) | **Rear Marker / Full Beam** (`lights.tail_light` / `full_beam`) | Truck energizes dedicated `full_beam` (100%) or steps `head_light` to 100%. Loco powers marker/tail. |
| **2** | `0x04` (4) | **Fog Lamp** (`lights.fog_lamp`) | **Fog / Marker Lamp** (`lights.fog_lamp`) | Independent forward fog illumination for trucks; auxiliary marker light for locomotives. |
| **3** | `0x08` (8) | **Hazard Lights** (`lights.turn_light`) | **Ditch Lights** (`lights.ditch_light`) | **Truck**: Flashes left and right turn indicators synchronously.<br>**Loco**: Alternating dual ditch lights. |
| **4** | `0x10` (16) | **Beacon Light** (`lights.beacon`) | **Beacon / Rotary Light** (`lights.beacon`) | Flashing roof strobe / rotating beacon pattern. |
| **5** | `0x20` (32) | **Cab Light** (`lights.cab_light`) | **Cab Light** (`lights.cab_light`) | Interior cabin illumination. |
| **6** | `0x40` (64) | **Work Light** (`lights.work_light`) | **Step / Ground Lights** (`lights.step_light`) | **Truck**: Working deck / rear floodlight.<br>**Loco**: Ground walkway step lights. |
| **7** | `0x80` (128)| **Aux Light** (`lights.aux_light`) | **Aux Light** (`lights.aux_light`) | General auxiliary lighting output. |

---

### 4.1 lights.head_light / tail_light

- `hardware`: the LED pin. **Required** for the light to be configured.
- `brightness_max`: maximum brightness in percent.
  **Type:** integer · **Default:** `60` · **Range:** 0–100.

`head_light` acts as the head light in road vehicles (energized when Head Light Bit 0 or High Beam Bit 1 is active). `tail_light` automatically tracks the live duty of `head_light` at 30% brightness.

Example:

```json
"head_light": {
    "hardware": "L1",
    "brightness_max": 60
},
"tail_light": {
    "hardware": "L2",
    "brightness_max": 60
}
```

### 4.2 lights.full_beam

Dedicated high-beam LED output. Energized when High Beam (Bit 1 / Item 1) is selected on the light selector. If `full_beam` is not configured, high beam falls back to driving `head_light` at 100% brightness.

- `hardware`: the LED pin. **Required** for the full beam to be configured.
- `brightness_max`: maximum brightness in percent.
  **Type:** integer · **Default:** `100` · **Range:** 0–100.

Example:

```json
"full_beam": {
    "hardware": "L6",
    "brightness_max": 100
}
```

### 4.3 lights.fog_lamp

Dedicated fog lamp output. Energized when Fog Lamp (Bit 2 / Item 2) is toggled on road vehicles.

- `hardware`: the LED pin. **Required** for the fog lamp to be configured.
- `brightness_max`: maximum brightness in percent.
  **Type:** integer · **Default:** `60` · **Range:** 0–100.

Example:

```json
"fog_lamp": {
    "hardware": "L7",
    "brightness_max": 100
}
```

### 4.4 lights.brake_light

Braking indicator output. Automatically energizes at 100% brightness when the brake pedal is pressed (> 20%) or during rapid deceleration braking.

- `hardware`: **Type:** string — either a dedicated **pin token** (`L0` … `L8`) or a **light alias** (e.g. `"tail_light"`).
- **Dual-Intensity Tail/Brake Mixing**: If `brake_light` shares a pin with `tail_light` (e.g. `"hardware": "tail_light"` or both assigned to the same pin), the pin runs at 30% duty for tail lighting when headlights are ON and automatically snaps to 100% duty when braking occurs.

> ⚠️ **`brightness_max` is ignored here.** The firmware forces the brake
> light to 100% brightness when active. A `brightness_max` key is accepted but has no effect.

Example (independent brake pin):

```json
"brake_light": {
    "hardware": "L3"
}
```

Example (shared dual-intensity tail/brake lamp):

```json
"brake_light": {
    "hardware": "tail_light"
}
```

### 4.5 lights.turn_light

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

### 4.6 lights.reversing_light

Independent reverse lamp output. Automatically energized when the vehicle shifts into Reverse (**Gear R**), or manually via **Bit 4** on the light selector.

- `hardware`: **Type:** string — a dedicated **pin token** (`L0` … `L8`).
- `brightness_max`: maximum brightness in percent. **Type:** integer · **Default:** `100` · **Range:** 0–100.

Example (dedicated white reversing lamp):

```json
"reversing_light": {
    "hardware": "L8",
    "brightness_max": 100
}
```

### 4.7 lights.beacon

Roof beacon or flashing strobe light, toggled via **Bit 4 (`0x10`)** on the light selector.

- `hardware`: **Type:** string — pin token (`L0` … `L8`).
- `brightness_max`: on-state brightness in percent. **Type:** integer · **Default:** `100` · **Range:** 0–100.

Example:

```json
"beacon": {
    "hardware": "L8",
    "brightness_max": 100
}
```

### 4.8 lights.cab_light

Interior cab illumination, toggled via **Bit 5 (`0x20`)** on the light selector.

- `hardware`: **Type:** string — pin token (`L0` … `L8`).
- `brightness_max`: on-state brightness in percent. **Type:** integer · **Default:** `40` · **Range:** 0–100.

### 4.9 lights.work_light / step_light

Working floodlight for road vehicles / trucks or ground walkway step lights for locomotives, toggled via **Bit 6 (`0x40`)** on the light selector.

- `hardware`: **Type:** string — pin token (`L0` … `L8`).
- `brightness_max`: on-state brightness in percent. **Type:** integer · **Default:** `80` (work), `30` (step) · **Range:** 0–100.

### 4.10 lights.aux_light

General auxiliary lighting output, toggled via **Bit 7 (`0x80`)** on the light selector.

- `hardware`: **Type:** string — pin token (`L0` … `L8`).
- `brightness_max`: on-state brightness in percent. **Type:** integer · **Default:** `60` · **Range:** 0–100.

### 4.11 lights.ditch_light

Locomotive dual ditch lights, toggled via **Bit 3 (`0x08`)** on locomotives. **Two outputs that flash alternately** — counter-phased, left on while right is off and vice versa.

The following parameters are available in the `lights.ditch_light` section:

- `left.hardware` / `right.hardware`: pin tokens for the two ditch outputs.
  **Type:** string · Allowed values: `L0` … `L8` (see
  [§8 Pin Reference](#8-pin-reference)).
- `interval_ms`: alternation half-period — how long **each** side stays lit
  before flipping. **Type:** integer · **Default:** `8` — alternation rate
  ≈ 1000/(2·interval) per second (`8` ≈ 60/s; `5`–`10` covers 50–100/s).
- `brightness_max`: on-state brightness (percent). **Type:** integer ·
  **Default:** `100` · **Range:** 0–100.

Example:

```json
"ditch_light": {
    "left":  { "hardware": "L4" },
    "right": { "hardware": "L5" },
    "brightness_max": 100,
    "interval_ms": 8
}
```

---

## 5. aux_motor / aux_light

Auxiliary work-machine outputs — the dump-truck tipper, cement-mixer drum, or
other add-on channels. Both are **optional**: absent from the hardware config,
no aux channel is initialized (no legacy auto-attached outputs exist anymore —
the old hardcoded S2/S3 aux servos are gone).

> ⚠️ **Skid-steer exclusion.** `aux_motor` is not usable on a skid-steer config
> ([§3 drivetrain](#3-drivetrain)) — the second motor output is the right
> track. The firmware logs `WARN: aux_motor: ignored in skid-steer mode` and
> leaves the aux channel unconfigured.

The **hardware config is the sole owner of aux wiring and purpose** — the
vehicle config never mentions pins, so the same `vehicle.json` works on any
board. Aux behavior (hydraulic flow sound, load governor) is keyed off aux
activity generically.

### 5.1 aux_motor

The aux motor channel. Electrically identical to
[`drivetrain.drive_motor`](#31-drivetraindrive_motor-left_motor-right_motor) —
the `hardware` token decides the output kind, so there is no separate
`aux_servo` key.

The following parameters are available in the `aux_motor` section:

- `hardware`: the output the motor is wired to. **Required** for the channel
  to be configured. Allowed values:
  - `DRIVER_A`, `DRIVER_B` — on-board motor drivers (H-bridge output)
  - `S1` … `S4` — a servo/ESC on a servo pin (PPM output, like the drive ESC)
- `frequency`: PWM frequency in Hz. **Type:** integer · **Default:** `20000`
  (driver) / `50` (servo/ESC output).
- `direction`: motor polarity. **Type:** string · **Default:** `forward` —
  same values as the drive motor (`forward`, `reverse`, `uni_forward`,
  `uni_reverse`).
- `duty.min` / `duty.max`: minimum/maximum duty percent applied at non-zero
  speed. **Type:** integer · **Default:** `20` / `90` · **Range:** 0–100.
- `type`: the aux *purpose*, which selects the app control profile.
  **Type:** string · **Default:** `mixer`.

### 5.2 aux_motor.type (mixer / tipper)

`type` is **purpose, not electrical kind** — the electrical kind (servo vs
H-bridge) is already derived from the `hardware` token. The type selects how
the app's `aux_slider` behaves:

| type | slider detents | slider centering | drive behavior |
|---|---|---|---|
| `mixer` | 5 (snap positions) | none (`RK_SPRING_NONE`) | proportional to position incl. direction; keeps running |
| `tipper` | 0 (continuous) | self-centering (`RK_SPRING_CENTER`) | momentary — follows the finger |
| `trailer_dcc` | — (deferred) | — | not implemented: channel unconfigured + boot warning |

> ⚠️ **`trailer_dcc` is deferred.** The DCC-like trailer control protocol is
> reserved in the schema enum but not yet implemented. A config declaring it
> loads fine but logs `WARN: aux_motor: type 'trailer_dcc' not yet
> implemented` and the channel stays unconfigured until a later change adds
> the protocol.

Example (cement-mixer drum on the second motor driver):

```json
"aux_motor": {
    "hardware": "DRIVER_B",
    "frequency": 20000,
    "direction": "forward",
    "duty": {
        "min": 20,
        "max": 90
    },
    "type": "mixer"
}
```

### 5.3 aux_light

Auxiliary work lamp on an LED channel. Same shape as
[`lights.head_light`](#41-lightshead_light-tail_light).

The following parameters are available in the `aux_light` section:

- `hardware`: the LED pin. **Required** for the light to be configured.
  Allowed values: `L0` … `L8` (see [§8 Pin Reference](#8-pin-reference)).
- `brightness_max`: on-state brightness (percent). **Type:** integer ·
  **Default:** `60` · **Range:** 0–100.

Example:

```json
"aux_light": {
    "hardware": "L6",
    "brightness_max": 60
}
```

---

## 6. animation

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

## 7. battery

LiPo pack configuration and voltage-sense calibration. All voltages are
**per cell**.

The following parameters are available in the `battery` section:

- `cell_count`: number of cells in the pack. **Type:** integer · **Default:**
  `0` (auto-detect) · **Range:** 0–4.
  - `0` — voltage-based auto-detection at boot (legacy behavior)
  - `1`–`4` — fixed cell count; the single source of truth for the pack
- `warning_voltage`: low-voltage warning threshold **per cell** (V).
  **Type:** float · **Default:** `3.5` · **Range:** 3.0–4.0.
- `cutoff_voltage`: low-voltage cutoff threshold **per cell** (V).
  **Type:** float · **Default:** `3.3` · **Range:** 3.0–3.8.
- `full_voltage`: fully-charged voltage **per cell** (V).
  **Type:** float · **Default:** `4.2`.
- `voltage_scale`: scale factor applied to the voltage sense reading.
  **Type:** float · **Default:** compile-time `VSCALE` (`1.8` for both
  shipped envs in `platformio.ini`).
- `voltage_offset`: offset added after scaling (volts).
  **Type:** float · **Default:** compile-time `VOFFSET` (`-0.2` for both
  shipped envs).

  When `voltage_scale`/`voltage_offset` are present they override the
  compile-time `VSCALE`/`VOFFSET` macros — useful for calibrating a specific
  board without rebuilding.

Example:

```json
"battery": {
    "cell_count": 1,
    "warning_voltage": 3.5,
    "cutoff_voltage": 3.3,
    "full_voltage": 4.2,
    "voltage_scale": 1.8,
    "voltage_offset": -0.2
}
```

---

## 8. power

Board power management, 3-state control (`OFF`, `ON`, `CHARGING`), and timing configuration. All time parameters are specified in **integer seconds** (`uint16_t`).

The following parameters are available in the `power` section:

- `hardware`: optional pin token (`"L0"`) or light alias (`"head_light"`, `"cab_light"`, etc.) for power status indicator & active button-hold feedback.
- `boot_latch_s`: boot button hold required to latch hardware power ON (s).
  **Type:** integer · **Default:** `1` · **Range:** 0–30.
- `button_hold_s`: continuous button hold duration to trigger graceful power-off (s).
  **Type:** integer · **Default:** `4` · **Range:** 1–30.
- `disconnect_timeout_s`: idle/disconnected auto power-off timeout (s).
  **Type:** integer · **Default:** `60` · **Range:** 0–3600 (`0` disables auto-off).
- `warning_window_s`: warning phase duration (hazard blinks & sound alert) before disconnect auto-off (s).
  **Type:** integer · **Default:** `10` · **Range:** 1–60.
- `cutoff_delay_s`: continuous low-voltage cutoff duration before power-off (s).
  **Type:** integer · **Default:** `2` · **Range:** 0–60.

Example:

```json
"power": {
    "hardware": "head_light",
    "boot_latch_s": 1,
    "button_hold_s": 4,
    "disconnect_timeout_s": 60,
    "warning_window_s": 10,
    "cutoff_delay_s": 2
}
```

> 💡 **3-State Board Behavior & Charging:**
> When charging is active (`CHARGE_SENS` pin HIGH), the board enters `CHARGING` state. Motor drive is disabled for safety, and the disconnect auto-off timer is suspended.
> During disconnection, single-clicking the physical power button resets the disconnect timer to 0, granting a fresh timeout window.

---

## 9. charging

Charging indicator channel and animation mode.

The following parameters are available in the `charging` section:

- `hardware`: pin token (`"L1"`) or light alias (`"head_light"`, `"cab_light"`, etc.) for the charging status indicator output.
- `mode`: animation mode during charging (`"solid"`, `"blink"`, or `"pulse"`). **Default:** `"solid"`.

Example:

```json
"charging": {
    "hardware": "head_light",
    "mode": "solid"
}
```

---

## 10. Pin Reference

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
| `DRIVER_A` | — | PWM1=18, PWM2=21, EN=17, BEMF=9 · dual-PWM bridge |
| `DRIVER_B` | — | PWM1=12, PWM2=13, EN=11, BEMF=10 · dual-PWM bridge |

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
| `DRIVER_A` | — | PWM1=13, PWM2=14, BEMF=4, common EN=12 · dual-PWM bridge |
| `DRIVER_B` | — | DIR=15, PWM=16, BEMF=5, common EN=12 · DIR+PWM bridge |

Note the per-board differences — e.g. `L1` is **GPIO 38** on MIKRO_V2 but
**GPIO 6** on TRACKLINK_V3, and `S2` is available on TRACKLINK_V3 only up to
`S2` (MIKRO_V2 goes to `S4`).

### 8.3 Hardware token vocabulary

- `L<n>` — LED channels, `L0`–`L8` (board-dependent upper bound).
- `S<n>` — servo/ESC channels, `S1`–`S4` (board-dependent upper bound).
- `DRIVER_A`, `DRIVER_B` — **semantic markers** for the on-board motor
  drivers, resolved per board to their PWM/DIR/EN/BEMF pins (tables above).
  `DRIVER_A` is a dual-PWM bridge on both boards; `DRIVER_B` is dual-PWM on
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
    "hardware": "DRIVER_A",         // dual-PWM bridge: PWM1=18, PWM2=21, EN=17
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
        "reversing_light": {
            "hardware": "brake_light"                  // alias: shares the brake lamp
        },
        "brake_light": {
            "hardware": "L3"                            // GPIO 40; brightness forced to 100
        },
        "turn_light": {
            "left":  { "hardware": "L4" },  // GPIO 41
            "right": { "hardware": "L5" },  // GPIO 42
            "brightness_max": 60,
            "type": "blink",                // accepted but ignored
            "interval_on": 500,             // ms
            "interval_off": 500             // ms
        }
    },
    "aux_motor": {
        "hardware": "DRIVER_B",             // mixer drum on the 2nd driver
        "frequency": 20000,
        "direction": "forward",
        "duty": {
            "min": 20,
            "max": 90
        },
        "type": "mixer"                     // mixer | tipper | trailer_dcc (deferred)
    },
    "aux_light": {
        "hardware": "L6",                   // work lamp
        "brightness_max": 60
    },
    "animation": {
        "easing_speed_deg_s": 180,          // 0 = instant
        "easing_k_in": 0.2,
        "easing_k_out": 0.8,
        "fade_duration_ms": 250
    },
    "battery": {
        "cell_count": 1,                    // 0 = auto-detect
        "cutoff_voltage": 3.4,              // per cell, V
        "full_voltage": 4.2,                // per cell, V
        "voltage_scale": 1.8,               // overrides compile-time VSCALE
        "voltage_offset": -0.2              // overrides compile-time VOFFSET
    }
}
```
