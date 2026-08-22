## 1. Config & Data Model Updates

- [x] 1.1 Add `uint8_t torqueConverterSlip = 100;` to `RcEngineSound::Config::Transmission` in `lib/SoundEngine/src/RcEngineSound.h`.
- [x] 1.2 Update `ConfigParser::parseTransmission()` in `common/ConfigParser.h` to parse `torque_converter_slip` / `TORQUE_CONVERTER_SLIP` (default 100).

## 2. Torque Converter Slip Calculation

- [x] 2.1 Compute `engineLoad = throttle - currentRpm` bounded to `[0, 180]` in `RcEngineSound::update()`.
- [x] 2.2 Calculate `converterSlip` with 2x launch multiplier in 1st/Rev gear and apply to `effectiveTarget` under `TRANS_AUTOMATIC`.
- [x] 2.3 Ensure engine RPM smoothly locks up to direct gear ratio when engine load reaches 0 at steady cruise.

## 3. Verification & Testing

- [x] 3.1 Add host unit test in `test/host_vc/` asserting torque converter RPM flaring on step throttle, launch slip boost, and lockup on speed stabilization.
- [x] 3.2 Verify build and end-to-end execution on live hardware via BLE Remote API.
