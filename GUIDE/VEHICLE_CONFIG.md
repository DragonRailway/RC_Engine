# RC Engine — Vehicle Config Reference

This document provides a description of all configuration options available
for the **vehicle config** (`vehicle-config.json` or `vehicle.json`). It is modeled after the
[Klipper configuration reference](https://www.klipper3d.org/Config_Reference.html).

The authoritative sources for this document are `common/ConfigParser.h` (parser implementation and key validation),
`lib/SoundEngine/src/RcEngineSound.h` (engine sound physics and simulation state),
`common/VehicleController.h` (vehicle control loop), and the shipped vehicle profiles under `configs/vehicle_configs/`.

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [vehicle](#2-vehicle)
3. [engine](#3-engine)
   - [Dynamics & RPM Curve](#31-dynamics--rpm-curve)
   - [Diesel Knock Simulator](#32-diesel-knock-simulator)
   - [Jake Brake (Engine Braking)](#33-jake-brake-engine-braking)
   - [Supercharger](#34-supercharger)
4. [transmission](#4-transmission)
5. [features](#5-features)
   - [Work Machine & Excavator Hydraulics](#51-work-machine--excavator-hydraulics)
   - [Track & Bucket Mechanics](#52-track--bucket-mechanics)
   - [Tire Squeal Physics](#53-tire-squeal-physics)
6. [sound_volumes](#6-sound_volumes)
   - [Primary Engine & Drive Volumes](#61-primary-engine--drive-volumes)
   - [Forced Induction & Mechanical FX](#62-forced-induction--mechanical-fx)
   - [Brakes, Transmission & Vehicle Alerts](#63-brakes-transmission--vehicle-alerts)
   - [Work Machine, Rail & Auxiliary Channels](#64-work-machine-rail--auxiliary-channels)
7. [mix_weights](#7-mix_weights)
8. [loop_points](#8-loop_points)
9. [Sound Sample Hierarchy & Resolution](#9-sound-sample-hierarchy--resolution)
10. [Examples](#10-examples)
    - [10.1 Highway Semi Truck (Scania V8)](#101-highway-semi-truck-scania-v8)
    - [10.2 Hydraulic Excavator (Caterpillar 323)](#102-hydraulic-excavator-caterpillar-323)
    - [10.3 Diesel-Electric Locomotive (EMD SD40-2)](#103-diesel-electric-locomotive-emd-sd40-2)

---

## 1. Introduction

The vehicle config defines **the vehicle's identity, engine physics, transmission characteristics, feature flags, loop points, and audio balance**. It is loaded at boot from `/vehicle-config.json` on the device LittleFS.

Unlike the hardware config (`hardware-<BOARD>.json`), which owns physical pins and board wiring, the vehicle config is **board-agnostic**: the exact same vehicle profile bundle (e.g. Scania V8 or CAT 320) can run on a `MIKRO_V2`, `TRACKLINK_V3`, or any custom PCB without changing a single line.

In the repository, profiles live in `configs/vehicle_configs/<VehicleName>/`:

```
configs/vehicle_configs/ScaniaV8/
├── vehicle.json
└── sounds/
    ├── start.json
    ├── idle.json
    ├── rev.json
    ├── knock.json
    ├── turbo.json
    └── ...
```

Vehicle configurations are packaged and deployed to LittleFS using `build_fs.py`:

```bash
python3 scripts/build_fs.py --board MIKRO_V2 --vehicle ScaniaV8
```

### How a Config is Read

- **Firmware Defaults**: Every parameter has a defined default. Any omitted section or key silently falls back to its default value.
- **Case-Insensitive Keys with Legacy Compatibility**: Both `snake_case` (recommended) and legacy `UPPER_CASE` keys are accepted.
- **Semantic Validation & Warnings**: The firmware validates keys, value ranges, and enums at boot and hot-reload. Invalid keys or out-of-range values log a `WARN:` line without halting execution.
- **Hot Reloading**: Editing or uploading `/vehicle-config.json` via RadioKit Web/BLE/Serial triggers hot reloading after write completion without requiring a hardware reboot.

---

## 2. vehicle

Declares vehicle identity, category, and sound set directory binding.

| Parameter | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `name` | string | `"Unknown"` | Human-readable vehicle name (e.g. `"Scania V8"`). Max 31 chars. |
| `description` | string | `""` | Optional vehicle description displayed in the UI. Max 127 chars. |
| `type` | string | `"TRUCK"` | Vehicle category (`"truck"`, `"locomotive"`, `"excavator"`). Selects the default app control surface and sound preset fallback directory. |
| `sound_set` | string | `name` | Name of the directory under `/sounds/vehicles/` containing sample JSONs. Defaults to `name` without spaces. |

```json
"vehicle": {
    "name": "Scania V8",
    "description": "Scania V8 Heavy Haul Semi Truck",
    "type": "truck",
    "sound_set": "ScaniaV8"
}
```

> 💡 **Vehicle Type Control Surfaces:**
> - `"truck"`: Activates Page 0 ("Truck") control surface with steering wheel, accelerator/brake pedals, and road lighting.
> - `"locomotive"`: Activates Page 1 ("Loco") control surface with notch throttle slider, directional reverser, and ditch/step lighting.
> - `"excavator"`: Uses truck surface while enabling auxiliary hydraulic pump and track rattle governor integrations.

---

## 3. engine

Configures the physics simulation of the internal combustion engine (RPM acceleration curve, inertia, rev limits, pitch scaling, diesel knock pattern, and jake braking).

### 3.1 Dynamics & RPM Curve

| Parameter | Type | Default | Range | Description |
| :--- | :--- | :--- | :--- | :--- |
| `acceleration` | integer | `2` | 1–50 | Rate at which engine RPM rises when throttle is applied. Higher values give faster rev response. |
| `deceleration` | integer | `2` | 1–50 | Rate at which engine RPM drops when throttle is released. |
| `inertia` | integer | `10` | 0–100 | Flywheel inertia weight dampening RPM transitions. |
| `max_pitch_factor` | float | `3.3` | 1.0–5.0 | Maximum playback rate multiplier for the `rev` sound sample at maximum RPM. |
| `rev_switch_point` | integer | `50` | 0–500 | RPM threshold where playback begins cross-fading from the `idle` sample to the `rev` sample. |
| `idle_end_point` | integer | `300` | 0–500 | RPM threshold where the `idle` sample fades out completely. |

```json
"engine": {
    "acceleration": 6,
    "deceleration": 4,
    "inertia": 10,
    "max_pitch_factor": 3.3,
    "rev_switch_point": 50,
    "idle_end_point": 300
}
```

### 3.2 Diesel Knock Simulator

Simulates cylinder combustion knocks by generating timed audio impulses synchronized with engine rotational speed.

| Parameter | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `knock_pattern` | string | `"v8"` | Firing order pattern (`"v8"`, `"v8_468"`, `"r6"`, `"r6_2"`, `"v2"`, `"uniform"`). |
| `diesel_knock_interval` | integer | `8` | Base interval step between knock pulses. |
| `knock_adaptive_volume` | integer | `18` | Dynamic volume boost per pulse applied under heavy throttle load. |
| `min_knock_volume` | integer | `80` | Minimum volume floor percentage for knock pulses. |
| `knock_start_rpm` | integer | `10` | RPM threshold above which knock pulses begin sounding. |

```json
"engine": {
    "knock_pattern": "v8",
    "diesel_knock_interval": 8,
    "knock_adaptive_volume": 18,
    "min_knock_volume": 80,
    "knock_start_rpm": 10
}
```

#### Supported Knock Patterns

| Pattern | Description | Accented Pulses |
| :--- | :--- | :--- |
| `"v8"` | Standard European / American V8 (Scania, Ford) | Accents positions 4 and 8 in an 8-pulse cycle |
| `"v8_468"` | Big block Chevy 468 V8 | Accents positions 1, 5, 9, 13 in a 16-pulse cycle |
| `"r6"` | Inline 6 cylinder diesel (Volvo, Cummins, CAT) | Accents position 6 in a 6-pulse cycle |
| `"r6_2"` | Inline 6 cylinder alternate firing rhythm | Accents positions 3 and 6 in a 6-pulse cycle |
| `"v2"` | 2-cylinder V-Twin (Harley-Davidson) | Accents positions 1 and 2 in a 4-pulse cycle |
| `"uniform"` | Equal intensity pulses (generic multi-cylinder) | All pulses uniform |

### 3.3 Jake Brake (Engine Braking)

Simulates heavy truck compression release engine brakes (Jake Brake).

| Parameter | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `jakebrake_min_rpm` | integer | `60` | Minimum RPM required for the Jake Brake sound and deceleration effect to engage. |
| `jakebrake_decel_rate` | integer | `5` | Additional RPM deceleration drag applied while the Jake Brake is active. |

```json
"engine": {
    "jakebrake_min_rpm": 60,
    "jakebrake_decel_rate": 5
}
```

### 3.4 Supercharger

| Parameter | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `supercharger_start_point` | integer | `10` | RPM threshold where supercharger whine sample starts playing. |

---

## 4. transmission

Configures vehicle gearbox simulation, shift transitions, and throttle ramp timing per gear.

| Parameter | Type | Default | Allowed Values | Description |
| :--- | :--- | :--- | :--- | :--- |
| `type` | string | `"none"` | `"none"`, `"automatic"`, `"manual"` | Gearbox simulation mode. |
| `number_of_gears` | integer | `3` | 1–6 | Number of forward gears. |
| `gear_ramp_times` | array[int] | `[20, 50, 75, 75, 75, 75]` | 0–255 | Throttle ramp slew delay (ms) per gear index (gears 1 to 6). |

```json
"transmission": {
    "type": "automatic",
    "number_of_gears": 3,
    "gear_ramp_times": [20, 50, 75, 75, 75, 75]
}
```

> 💡 **Transmission Modes:**
> - `"automatic"`: Automatically shifts gears based on engine RPM and road load, triggering torque converter RPM drops and the `shifting` sound sample.
> - `"manual"`: Gear changes are triggered manually by shifting events.
> - `"none"`: Direct drive / single-speed (standard for locomotives and simple RC models).

---

## 5. features

Enables specialized sound synthesizers and physics modules for heavy machinery, excavators, dump trucks, and road vehicles.

### 5.1 Work Machine & Excavator Hydraulics

| Parameter | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `hydraulic_enabled` | boolean | `false` | Enables hydraulic pump and fluid flow sound loops driven by auxiliary channel activity. |
| `hydrostatic_mode` | boolean | `false` | Enables continuous hydrostatic drive pump sound loop. |

```json
"features": {
    "hydraulic_enabled": true,
    "hydrostatic_mode": false
}
```

### 5.2 Track & Bucket Mechanics

| Parameter | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `track_rattle_enabled` | boolean | `false` | Enables speed-dependent steel track rattle for tracked vehicles and excavators. |
| `track_rattle_interval_min` | integer | `90` | Minimum interval (ms) between track rattle clicks at top speed. |
| `track_rattle_interval_max` | integer | `500` | Maximum interval (ms) between track rattle clicks at crawl speed. |
| `dump_bed_enabled` | boolean | `false` | Enables dump bed hydraulic ram sound effects. |

```json
"features": {
    "track_rattle_enabled": true,
    "track_rattle_interval_min": 90,
    "track_rattle_interval_max": 500,
    "dump_bed_enabled": false
}
```

### 5.3 Tire Squeal Physics

| Parameter | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `tire_squeal_threshold` | integer | `70` | Steering angle threshold (percent, 0–100) required to trigger tire squeal audio. |
| `tire_squeal_max_speed` | integer | `30` | Speed threshold percentage required before high-speed tire screeching sounds. |

```json
"features": {
    "tire_squeal_threshold": 70,
    "tire_squeal_max_speed": 30
}
```

---

## 6. sound_volumes

Controls individual gain levels for all 32 audio slots in the sound engine. Values are specified in **percent** (typically 0–200%, where `100` = 100% nominal volume, `0` = muted).

### 6.1 Primary Engine & Drive Volumes

| Parameter | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `start` | integer | `100` | Engine starter motor cranking and ignition sequence volume. |
| `idle` | integer | `100` | Base engine idle loop volume. |
| `idle_min` | integer | `0` | Minimum idle volume floor when revving under throttle. |
| `rev` | integer | `100` | Base engine rev / acceleration loop volume. |
| `rev_min` | integer | `0` | Minimum rev volume floor at zero throttle. |
| `full_throttle` | integer | `100` | Dynamic volume multiplier added at 100% throttle position. |

### 6.2 Forced Induction & Mechanical FX

| Parameter | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `turbo` | integer | `0` | Turbocharger spooling whistle loop volume. |
| `turbo_min` | integer | `0` | Minimum turbo volume at low RPM. |
| `wastegate` | integer | `0` | Turbo wastegate blow-off pop volume on sudden throttle release. |
| `wastegate_min` | integer | `0` | Minimum wastegate volume. |
| `supercharger` | integer | `0` | Roots/twin-screw supercharger whine volume. |
| `supercharger_min` | integer | `10` | Idle supercharger whine volume. |
| `knock` | integer | `0` | Diesel combustion knock volume. |
| `knock_min` | integer | `0` | Minimum diesel knock volume at idle. |
| `fan` | integer | `0` | Radiator cooling fan sound volume. |

### 6.3 Brakes, Transmission & Vehicle Alerts

| Parameter | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `horn` | integer | `100` | Main air horn / train horn volume. |
| `siren` | integer | `0` | Emergency beacon siren volume. |
| `brake` | integer | `0` | Air brake release hiss and friction squeal volume. |
| `parking_brake` | integer | `0` | Spring brake air discharge volume during engine shutdown. |
| `jakebrake` | integer | `0` | Jake brake compression exhaust volume. |
| `jakebrake_min` | integer | `0` | Minimum jake brake volume floor. |
| `shifting` | integer | `0` | Pneumatic gear shift clunk and transmission whine volume. |
| `reversing` | integer | `0` | Backup beeper alert volume. |
| `indicator` | integer | `0` | Turn indicator flasher relay click volume. |
| `coupling` | integer | `0` | Trailer / fifth-wheel coupling clunk volume. |
| `uncoupling` | integer | `0` | Trailer release sound volume. |

### 6.4 Work Machine, Rail & Auxiliary Channels

| Parameter | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `hydraulic_pump` | integer | `0` | Hydraulic pump motor whine volume. |
| `hydraulic_flow` | integer | `0` | High-pressure fluid hiss through control valves volume. |
| `track_rattle` | integer | `0` | Steel track link rattle volume. |
| `bucket_rattle` | integer | `0` | Metal excavator bucket / dump bed rattle volume. |
| `bell` | integer | `0` | Locomotive warning bell volume. |
| `whistle` | integer | `0` | Steam / locomotive whistle volume. |
| `door` | integer | `0` | Cab door slam / air door actuation volume. |
| `tire_squeal` | integer | `0` | Cornering tire screech volume. |
| `scanner` | integer | `0` | KITT scanner / auxiliary sweep volume. |
| `music` | integer | `0` | Background audio / ice cream truck chime volume. |
| `gun` | integer | `0` | Auxiliary combat FX volume. |
| `out_of_fuel` | integer | `0` | Engine sputter / fuel starvation volume. |
| `sound1` | integer | `100` | General-purpose user sound channel volume. |
| `others` | integer | `0` | Custom auxiliary sound slot volume. |
| `crawler_mode_threshold` | integer | `44` | Micro-throttle volume threshold for trial/crawler crawling. |

Example:

```json
"sound_volumes": {
    "start": 140,
    "idle": 80,
    "full_throttle": 150,
    "rev": 100,
    "turbo": 40,
    "knock": 200,
    "wastegate": 100,
    "horn": 100,
    "brake": 150,
    "parking_brake": 150,
    "shifting": 100,
    "reversing": 70,
    "jakebrake": 150,
    "indicator": 100
}
```

---

## 7. mix_weights

Adjusts the global balance between the continuous engine simulation (idle, rev, turbo, knock, fan) and one-shot / triggered sound effects (horns, sirens, brakes, shifts, hydraulics).

| Parameter | Type | Default | Range | Description |
| :--- | :--- | :--- | :--- | :--- |
| `engine` | integer | `100` | 0–100 | Master mix weight percent for engine voices. |
| `effects` | integer | `100` | 0–100 | Master mix weight percent for sound effect voices. |

```json
"mix_weights": {
    "engine": 100,
    "effects": 100
}
```

---

## 8. loop_points

Specifies sample loop boundaries (in PCM sample offsets) for sustained sound effects. When a trigger is held active, playback plays from sample `0` to `<slot>_end`, then loops continuously between `<slot>_begin` and `<slot>_end` until released.

| Parameter | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `horn_begin` / `horn_end` | integer | `0`, `0` | Loop start / end sample index for `horn.json`. |
| `siren_begin` / `siren_end` | integer | `0`, `0` | Loop start / end sample index for `siren.json`. |
| `reversing_begin` / `reversing_end` | integer | `0`, `0` | Loop start / end sample index for `reversing.json`. |
| `sound1_begin` / `sound1_end` | integer | `0`, `0` | Loop start / end sample index for `sound1.json`. |

```json
"loop_points": {
    "horn_begin": 10500,
    "horn_end": 28400,
    "siren_begin": 0,
    "siren_end": 0,
    "reversing_begin": 0,
    "reversing_end": 0,
    "sound1_begin": 0,
    "sound1_end": 0
}
```

> Setting `begin: 0` and `end: 0` loops the entire sample buffer from start to finish.

---

## 9. Sound Sample Hierarchy & Resolution

When the sound engine initializes a vehicle, it searches LittleFS for sample JSON files for each of the 32 sound slots using a **3-tier fallback hierarchy**:

```
┌────────────────────────────────────────────────────────────────┐
│  Tier 1: Dedicated Vehicle Sound Set                           │
│  /sounds/vehicles/<sound_set>/<slot>.json                      │
└──────────────────────────────┬─────────────────────────────────┘
                               │ (if not found)
                               ▼
┌────────────────────────────────────────────────────────────────┐
│  Tier 2: Category Sound Preset                                 │
│  /sounds/common/<type>/<slot>.json                             │
│  /sounds/presets/<type>/<slot>.json                            │
└──────────────────────────────┬─────────────────────────────────┘
                               │ (if not found)
                               ▼
┌────────────────────────────────────────────────────────────────┐
│  Tier 3: Generic / Universal Sound Bank                        │
│  /sounds/generic/<slot>.json                                   │
└────────────────────────────────────────────────────────────────┘
```

### Supported Sound Slots

| Slot Name | Typical Sample Source | Trigger Condition |
| :--- | :--- | :--- |
| `start` | Starter motor & engine fire | Engine toggle ON (`start_button` / `engine_button`) |
| `idle` | Base engine idle loop | Engine running at minimum throttle |
| `rev` | High-RPM engine roar loop | Throttle applied above `rev_switch_point` |
| `knock` | Diesel combustion pulse | Synchronized with engine knock pattern |
| `turbo` | Turbo whistle loop | Rises proportionally with engine load |
| `wastegate` | Blow-off valve dump | Rapid drop in throttle position |
| `horn` | Air horn / electric horn | Horn button pressed |
| `jakebrake` | Heavy exhaust compression | Engine braking active above `jakebrake_min_rpm` |
| `fan` | Engine cooling fan | High engine load / temperature |
| `siren` | Emergency siren | Beacon/siren light bit active |
| `brake` | Air brake release hiss | Brake pedal release / decel brake event |
| `parking_brake`| Spring brake release / set | Engine shutdown sequence |
| `shifting` | Gear shift pneumatic clunk | Transmission gear changes |
| `reversing` | Backup beeper tone | Gear shifted to Reverse (`R`) |
| `indicator` | Relay flasher tick | Turn signals or hazard lights active |
| `coupling` | Fifth wheel latch | Coupling trigger active |
| `uncoupling` | Pin release sound | Uncoupling trigger active |
| `hydraulic_pump`| Hydraulic pump whine | Work machine auxiliary motor / hydraulic activity |
| `hydraulic_flow`| Fluid rushing sound | Work machine actuator in motion |
| `track_rattle` | Metal track clatter | Tracked vehicle in motion |
| `bucket_rattle`| Bucket shake sound | Bucket rattle action triggered |
| `bell` | Warning bell | Locomotive bell button active |
| `door` | Cab door slam | Vehicle connect / enter action |
| `sound1` | User sound | Custom trigger |

---

## 10. Examples

### 10.1 Highway Semi Truck (Scania V8)

```json
{
  "vehicle": {
    "name": "Scania V8",
    "description": "Scania V8 Heavy Haul Semi Truck",
    "type": "truck",
    "sound_set": "ScaniaV8"
  },
  "engine": {
    "acceleration": 6,
    "deceleration": 4,
    "inertia": 10,
    "max_pitch_factor": 3.3,
    "rev_switch_point": 50,
    "idle_end_point": 40,
    "knock_pattern": "v8",
    "diesel_knock_interval": 8,
    "knock_adaptive_volume": 18,
    "min_knock_volume": 80,
    "knock_start_rpm": 10,
    "jakebrake_min_rpm": 60,
    "jakebrake_decel_rate": 5,
    "supercharger_start_point": 10
  },
  "transmission": {
    "type": "automatic",
    "number_of_gears": 3,
    "gear_ramp_times": [20, 50, 75, 75, 75, 75]
  },
  "mix_weights": {
    "engine": 100,
    "effects": 100
  },
  "features": {
    "tire_squeal_threshold": 70,
    "tire_squeal_max_speed": 30,
    "hydraulic_enabled": false,
    "hydrostatic_mode": false,
    "track_rattle_enabled": false,
    "dump_bed_enabled": false
  },
  "sound_volumes": {
    "start": 140,
    "idle": 80,
    "full_throttle": 150,
    "rev": 100,
    "turbo": 40,
    "knock": 200,
    "wastegate": 100,
    "horn": 100,
    "siren": 100,
    "brake": 150,
    "parking_brake": 150,
    "shifting": 100,
    "reversing": 70,
    "indicator": 100,
    "coupling": 100,
    "jakebrake": 150
  }
}
```

---

### 10.2 Hydraulic Excavator (Caterpillar 323)

```json
{
  "vehicle": {
    "name": "Caterpillar 323",
    "description": "Caterpillar 323 Hydraulic Excavator",
    "type": "excavator",
    "sound_set": "Caterpillar323"
  },
  "engine": {
    "acceleration": 3,
    "deceleration": 2,
    "knock_pattern": "r6"
  },
  "transmission": {
    "type": "none"
  },
  "features": {
    "hydraulic_enabled": true,
    "hydrostatic_mode": false,
    "track_rattle_enabled": true,
    "track_rattle_interval_min": 90,
    "track_rattle_interval_max": 500
  },
  "sound_volumes": {
    "start": 140,
    "idle": 90,
    "rev": 110,
    "horn": 100,
    "hydraulic_pump": 80,
    "hydraulic_flow": 70,
    "track_rattle": 90,
    "bucket_rattle": 100
  }
}
```

---

### 10.3 Diesel-Electric Locomotive (EMD SD40-2)

```json
{
  "vehicle": {
    "name": "EMD SD40-2",
    "description": "EMD SD40-2 Diesel-Electric Locomotive",
    "type": "locomotive",
    "sound_set": "EMDSD40-2DieselLoco"
  },
  "engine": {
    "acceleration": 2,
    "deceleration": 1,
    "inertia": 40,
    "max_pitch_factor": 2.5,
    "knock_pattern": "uniform"
  },
  "transmission": {
    "type": "none"
  },
  "loop_points": {
    "horn_begin": 8500,
    "horn_end": 32000
  },
  "sound_volumes": {
    "start": 120,
    "idle": 100,
    "rev": 110,
    "horn": 140,
    "bell": 100,
    "brake": 120,
    "reversing": 60
  }
}
```
