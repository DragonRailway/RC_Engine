## Design

### D1 — Canonical VehicleType enum

`RcEngineSound::VehicleType` becomes the single canonical enum, gaining `VEHICLE_UNKNOWN`:

```cpp
enum VehicleType {
    VEHICLE_TRUCK,
    VEHICLE_LOCOMOTIVE,
    VEHICLE_EXCAVATOR,
    VEHICLE_UNKNOWN      // unrecognized config string → safe fallback
};
```

- The dead top-level `VehicleType` in `lib/SoundEngine/src/SoundTypes.h` is deleted (only the `VehicleType` block — `SoundID`, `SoundSlot`, `SoundData` stay).
- `VEHICLE_CUSTOM` is removed (no consumer; unknown strings now resolve to `UNKNOWN`, which is the honest representation). `VEHICLE_CAR`/`VEHICLE_TANK` never existed in the live enum.

### D2 — Config parsing with validation

`ConfigParser::loadVehicleConfig` maps the type string with an explicit final else → `VEHICLE_UNKNOWN` plus a loud boot warning:

```cpp
const char* vehicleTypeStr = vehObj["type"] | vehObj["TYPE"] | "TRUCK";
if (strcasecmp(vehicleTypeStr, "LOCOMOTIVE") == 0) {
    config.type = RcEngineSound::VEHICLE_LOCOMOTIVE;
} else if (strcasecmp(vehicleTypeStr, "EXCAVATOR") == 0) {
    config.type = RcEngineSound::VEHICLE_EXCAVATOR;
} else if (strcasecmp(vehicleTypeStr, "TRUCK") == 0) {
    config.type = RcEngineSound::VEHICLE_TRUCK;
} else {
    config.type = RcEngineSound::VEHICLE_UNKNOWN;
    Serial.printf("[ConfigParser] WARNING: unknown vehicle type '%s' — defaulting to truck\n", vehicleTypeStr);
}
```

No silent TRUCK fallback: an unrecognized string is visible in the boot log.

### D3 — Type dispatch in the controller

`VehicleController::update()` derives behavior from the enum directly:

```cpp
const RcEngineSound::VehicleType vtype = s_profile->config.type;
const bool isLoco = (vtype == RcEngineSound::VEHICLE_LOCOMOTIVE);
const bool isExcavator = (vtype == RcEngineSound::VEHICLE_EXCAVATOR);  // stub
```

- `isLoco` gates the loco widget set exactly as today.
- `isExcavator` is a recognized stub: it logs once at boot that the excavator control surface is deferred and uses the truck widget path. All existing `isLoco ? … : …` branches stay untouched — excavator and unknown naturally fall into the truck side of each ternary.
- `printConfig` already prints the numeric type; the boot log line for the excavator stub is added in `init()` or `update()`'s first pass.

### D4 — App-side isolation from `main.cpp`

Set after `initRadioKit()` in `setup()` (runtime override — no header re-patch needed on UI re-imports):

```cpp
initRadioKit();

// Device type string matches the configured vehicle type
RadioKit.config.type = (profile.config.type == RcEngineSound::VEHICLE_LOCOMOTIVE) ? "Locomotive" : "Truck";

// Land the app on the correct page for this model
RadioKit.setActivePage(profile.config.type == RcEngineSound::VEHICLE_LOCOMOTIVE ? 1 : 0);
```

- `RadioKit.config.type` is informational (never read by the lib firmware-side); setting it post-`begin()` is safe and the app reads it on connect.
- `setActivePage()` sends `CMD_PAGE_SWITCH` and bakes the active page into the initial `CONF_DATA`, so the app lands on the right page even before the user interacts. It's a nudge, not a lock — app-initiated page switches still work afterwards (UI-design concern, out of scope).

### D5 — Behavior matrix

| config `type` | enum | widget set | app page | device type | boot log |
|---|---|---|---|---|---|
| `truck` | TRUCK | truck | 0 "Truck" | "Truck" | — |
| `locomotive` | LOCOMOTIVE | loco | 1 "Loco" | "Locomotive" | — |
| `excavator` | EXCAVATOR | truck (stub) | 0 "Truck" | "Truck" | stub note |
| anything else | UNKNOWN | truck | 0 "Truck" | "Truck" | warning |

### Files touched

- `lib/SoundEngine/src/SoundTypes.h` — remove dead `VehicleType` enum block
- `lib/SoundEngine/src/RcEngineSound.h` — canonical enum: drop `CUSTOM`, add `VEHICLE_UNKNOWN`
- `common/ConfigParser.h` — type parse → UNKNOWN + warning
- `common/VehicleController.h` — dispatch on enum, excavator stub log
- `src/main.cpp` — device type string + forced active page after `initRadioKit()`
