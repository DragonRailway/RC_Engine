## Why

Vehicle type isolation is incomplete. Today `config.type` from `/vehicle-config.json` is consumed in exactly one place — the binary `isLoco` boolean in `VehicleController::update()`. Meanwhile:

- `ConfigParser` parses **four** types (`LOCOMOTIVE`, `EXCAVATOR`, `CUSTOM`, else TRUCK) but the controller only distinguishes LOCOMOTIVE vs *everything else*. An excavator config (`vehicle-Caterpillar323.json`) silently boots into the truck widget path — gas pedal, steering wheel, gear D/P/R — even though it's a tracked machine with hydraulic features.
- Two divergent `VehicleType` enums exist: the live nested `RcEngineSound::VehicleType` (`TRUCK/LOCOMOTIVE/EXCAVATOR/CUSTOM`) and a dead top-level one in `SoundTypes.h` (`TRUCK/EXCAVATOR/LOCOMOTIVE/CAR/TANK/UNKNOWN`). Same concept, two definitions, different members — a maintenance hazard.
- The app side is not isolated at all: `src/RADIOKIT.h` hardcodes `RadioKit.config.type = "Locomotive"`, and the RadioKit app always shows both Truck and Loco pages regardless of the configured vehicle type.

The two real control surfaces are **TRUCK** and **LOCOMOTIVE** (one model per device, never both). EXCAVATOR is a recognized but deferred type. This change makes the type axis honest: one canonical enum, correct parsing with validation, type-aware boot behavior for the app, and an explicit excavator stub instead of silent fallthrough.

## What Changes

- **Unify the vehicle type enum**: make `RcEngineSound::VehicleType` the single canonical enum (`TRUCK`, `LOCOMOTIVE`, `EXCAVATOR`, `UNKNOWN`), delete the dead `SoundTypes.h` `VehicleType`, and map unrecognized config strings to `UNKNOWN` (with a boot warning) instead of silently defaulting to TRUCK.
- **Type dispatch in the controller**: replace the bare `isLoco` comparison with a proper dispatch. LOCOMOTIVE → loco widget set; TRUCK → truck widget set; EXCAVATOR → recognized stub that temporarily uses the truck widget path with an explicit boot log (control surface deferred to a future change); UNKNOWN → truck fallback with a warning.
- **App-side isolation**: after `initRadioKit()`, force the active page from vehicle type (`RadioKit.setActivePage(isLoco ? 1 : 0)`) so the app lands on the right page on connect, and drive `RadioKit.config.type` from the vehicle config (`"Truck"` / `"Locomotive"`) instead of the hardcoded `"Locomotive"`.
- **Validation visibility**: an unrecognized `type` string is logged loudly at boot (`UNKNOWN type — defaulting to truck`) so config typos are visible.

## Capabilities

### New Capabilities
None — no new capability is introduced; this hardens existing ones.

### Modified Capabilities
- `vehicle-control-loop`: "Throttle and steering mapping" gains UNKNOWN-type handling and an explicit excavator stub behavior.
- `radiokit-ble-control`: "RadioKit lifecycle" is modified so the device type string and active page are driven by the vehicle config type.

## Impact

- `lib/SoundEngine/src/SoundTypes.h` — remove dead `VehicleType` enum (keep `SoundID`, `SoundSlot`, `SoundData`).
- `lib/SoundEngine/src/RcEngineSound.h` — canonical `VehicleType` gains `VEHICLE_UNKNOWN`; `CUSTOM` removed or remapped.
- `common/ConfigParser.h` — map unknown type strings to `UNKNOWN` with a warning; `CUSTOM` handling revisited.
- `common/VehicleController.h` — type dispatch (loco / truck / excavator stub / unknown).
- `src/main.cpp` — set `RadioKit.config.type` from vehicle config and force the active page after `initRadioKit()`.
- `src/RADIOKIT.h` — no change needed if `main.cpp` overrides the hardcoded type at runtime.
- Config files (`data/vehicle-*.json`) — unchanged; Caterpillar323 keeps `"type": "excavator"` and boots via the stub.
