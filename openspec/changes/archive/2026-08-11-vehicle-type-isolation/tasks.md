## 1. Enum Unification

- [x] 1.1 Remove the dead `VehicleType` enum block from `lib/SoundEngine/src/SoundTypes.h` (keep `SoundID`, `SoundSlot`, `SoundData`)
- [x] 1.2 Update `RcEngineSound::VehicleType` in `lib/SoundEngine/src/RcEngineSound.h`: drop `VEHICLE_CUSTOM`, add `VEHICLE_UNKNOWN` (canonical: TRUCK, LOCOMOTIVE, EXCAVATOR, UNKNOWN)
- [x] 1.3 Audit for any remaining `VEHICLE_CUSTOM` / `VehicleType` references across the codebase and migrate or remove them

## 2. Config Parsing & Controller Dispatch

- [x] 2.1 `ConfigParser::loadVehicleConfig`: map unknown `type` strings to `VEHICLE_UNKNOWN` with a loud boot warning (no silent TRUCK fallback); keep LOCOMOTIVE/EXCAVATOR/TRUCK mappings
- [x] 2.2 `VehicleController`: dispatch on the canonical enum — `isLoco` (LOCOMOTIVE) as today; `isExcavator` stub (truck widget path) with a one-time boot log noting the deferred control surface; UNKNOWN uses the truck path
- [x] 2.3 Verify `printConfig` still prints the type (numeric) and add any missing boot-log visibility for the stub/unknown paths

## 3. App-Side Isolation

- [x] 3.1 `src/main.cpp`: after `initRadioKit()`, set `RadioKit.config.type` to `"Locomotive"` or `"Truck"` from the vehicle config type
- [x] 3.2 `src/main.cpp`: call `RadioKit.setActivePage(loco ? 1 : 0)` after init so the app lands on the correct page on connect

## 4. Validation

- [x] 4.1 Build `pio run -e TRACKLINK_V3` and `pio run -e MIKRO_V2` with no errors
- [x] 4.2 Review the diff against the delta specs (each scenario has matching behavior in code) — reviewer fixes: removed dead isExcavator in update(), made the app device-type scenario truthful about the vendored lib not serializing config.type
