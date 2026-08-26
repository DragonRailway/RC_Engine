## Context

The RC_brain system runs on ESP32-S3 boards (such as `MIKRO_V2` and `TRACKLINK_V3`) with LittleFS storing the active hardware config, vehicle config, and sound assets. The `TRACKLINK_V3` board is dedicated to rail and tracked configurations with DRIVER_A (PWM motor drive), directional headlights/tail lights, dual ditch lights (alternating strobe), cab light, and step light.

An Android tablet running the RadioKit companion app is connected over ADB with port forwarding (`adb forward tcp:17007 tcp:7007`). The RadioKit app exposes a REST API at `http://127.0.0.1:17007/api` allowing complete remote inspection, BLE device connection, filesystem management (`/api/fs/*`), and widget control manipulation over BLE.

## Goals / Non-Goals

**Goals:**
- Provide a validated `UnionPacific2002/vehicle.json` config bundle for EMD SD70M locomotive simulation (acc/dec=1 mass inertia, direct drive transmission, EMD 16-710 diesel sound mix).
- Validate staging and build size of `TRACKLINK_V3` + `UnionPacific2002` using `scripts/build_fs.py`.
- Use the RadioKit Remote API (`/api/fs/*`, `/api/widgets`, `/api/connection`) over BLE to manage the LittleFS on the ESP32 via the tablet, verify hot-reload, and test locomotive control widgets.

**Non-Goals:**
- Modifying RadioKit Flutter app source code or creating new BLE transports.
- Changing pin mappings in `boards/TRACKLINK_V3.h`.

## Decisions

### Decision 1: Create `configs/vehicle_configs/UnionPacific2002/vehicle.json`
- **Choice**: Match the parameters from `sounds/raw_vehicles/UnionPacific2002.h`. Set `type: "locomotive"`, `sound_set: "UnionPacific2002"`, `transmission.type: "direct"`, and heavy engine mass inertia (`acc: 1, dec: 1`).
- **Rationale**: `UnionPacific2002` has complete EMD sound assets in `configs/vehicle_configs/UnionPacific2002/sounds/` and aligns with `ConfigParser` locomotive expectations.
- **Alternatives**: Creating a new named directory `EMD_SD70M` would require copying 17 sound files (~600 KB); using `UnionPacific2002` leverages existing assets directly.

### Decision 2: Remote Filesystem & Control via ADB Port Forwarding
- **Choice**: Forward port `17007` to Android app port `7007` using `adb forward tcp:17007 tcp:7007`. Use curl/Python scripts against `http://127.0.0.1:17007/api/` for BLE pairing/connecting, reading/writing `/hardware-TRACKLINK_V3.json` and `/vehicle-config.json`, and issuing widget commands (`throttle_slider`, `engine_button`, `loco_light`).
- **Rationale**: Avoids manual physical flashing or UI clicking; enables deterministic testing directly from the host.

## Risks / Trade-offs

- **[Risk]** Android tablet app process terminates or ADB disconnects during BLE FS transfer.
  → *Mitigation:* Ensure `adb forward tcp:17007 tcp:7007` is active and verify `GET /api/status` and `GET /api/connection` before initiating FS operations.
- **[Risk]** Large sound files over BLE LittleFS transfer could be slow.
  → *Mitigation:* For initial deployment, flash filesystem directly via `build_fs.py` or upload configs via `/api/fs/upload`. Hot-reloads can modify `vehicle-config.json` and `hardware-TRACKLINK_V3.json` in sub-second time.
