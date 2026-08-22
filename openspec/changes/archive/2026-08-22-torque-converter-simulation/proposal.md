## Why

Currently, `RcEngineSound`'s automatic transmission simulation (`TRANS_AUTOMATIC`) uses a linear geometric sawtooth mapping between `virtualSpeed` and engine RPM slices. Real heavy trucks, construction machinery, and automatic road vehicles utilize hydraulic torque converters that slip under load—producing an instantaneous RPM flare on launch and heavy throttle transients that smoothly settles as vehicle momentum builds and the fluid coupling locks up. Introducing load-dependent torque converter slip provides realistic powertrain audio dynamics and matches the reference standard (`Rc_Engine_Sound_ESP32`).

## What Changes

- **Engine Load Calculation**: Introduce dynamic engine load tracking (`engineLoad = throttle - currentRpm`, bounded to `[0, 180]`) in `RcEngineSound.cpp`.
- **Load-Dependent Torque Converter Slip**: Compute converter slip flare (`converterSlip = engineLoad * slipPercentage / 100`) added to target RPM in `TRANS_AUTOMATIC`.
- **Launch Gear Slip Multiplication**: Apply 2x slip multiplier in 1st and Reverse gears to simulate torque multiplication and stall speed on launch.
- **Config & JSON Schema Expansion**: Add `torque_converter_slip` / `TORQUE_CONVERTER_SLIP` parameter (default 100%) to `transmission` config block in `RcEngineSound.h` and `ConfigParser.h`.
- **Host VC Test Suite**: Add comprehensive unit tests verifying torque converter flaring, launch slip multiplier, and high-speed lockup.

## Capabilities

### New Capabilities
- `torque-converter-slip`: Load-dependent torque converter fluid coupling simulation for automatic transmissions with 1st/Rev gear launch boost and high-speed lockup.

### Modified Capabilities
- None.

## Impact

- `RcEngineSound.h`: Add `uint8_t torqueConverterSlip = 100;` to `struct Transmission`.
- `RcEngineSound.cpp`: Add engine load and converter slip calculation in `update()` under `TRANS_AUTOMATIC`.
- `ConfigParser.h`: Parse `torque_converter_slip` in `parseTransmission()`.
- `test/host_vc/`: New assertions validating RPM flaring during acceleration and lockup when speed stabilizes.
