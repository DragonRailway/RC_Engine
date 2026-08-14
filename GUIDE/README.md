# RC Brain — Configuration Guide

User-facing guides for configuring the RC Brain firmware. Modeled after the
[Klipper configuration reference](https://www.klipper3d.org/Config_Reference.html)
style: one page per config area, every parameter documented with its type,
default, allowed values, and notes.

## Hardware config

- [Hardware Config Reference](HARDWARE_CONFIG.md) — the `hardware-<BOARD>.json`
  schema: every section, parameter, and default, plus per-board pin tables
  and a worked example.

## Vehicle config

- [Vehicle Config Reference](VEHICLE_CONFIG.md) — the `vehicle.json` schema
  (engine, transmission, sound volumes, features). Currently a stub; the
  authoritative sources are `common/ConfigParser.h` and the shipped configs
  under `configs/vehicle_configs/`.

## Where configs live

| Repo | Deployed to (device LittleFS) | Purpose |
|---|---|---|
| `configs/hardware_configs/hardware-<BOARD>.json` | `/hardware-config.json` | Pins, drivetrain, lights, animation, battery |
| `configs/vehicle_configs/<set>/vehicle.json` | `/vehicle-config.json` | Sound engine: vehicle identity, engine, transmission, volumes |
| `configs/vehicle_configs/<set>/sounds/*.json` | `/sounds/vehicles/<set>/*.json` | Sound samples (PCM) |

The board is selected at compile time (`-D MIKRO_V2` or `-D TRACKLINK_V3`),
and the matching hardware config is flashed to the filesystem with
`scripts/build_fs.py --board <BOARD> --vehicle <SET>`.
