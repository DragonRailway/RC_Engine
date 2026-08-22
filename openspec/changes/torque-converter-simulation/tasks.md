## 1. Config & Data Model Updates

- [ ] 1.1 Add `uint8_t torqueConverterSlip = 100;` to `RcEngineSound::Config::Transmission` in `lib/SoundEngine/src/RcEngineSound.h`.
- [ ] 1.2 Update `ConfigParser::parseTransmission()` in `common/ConfigParser.h` to parse `torque_converter_slip` / `TORQUE_CONVERTER_SLIP` (default 100).

## 2. Torque Converter Slip Calculation

- [ ] 2.1 Compute `engineLoad = throttle - currentRpm` bounded to `[0, 180]` in `RcEngineSound::update()`.
- [ ] 2.2 Calculate `converterSlip` with 2x launch multiplier in 1st/Rev gear and apply to `effectiveTarget` under `TRANS_AUTOMATIC`.
- [ ] 2.3 Ensure engine RPM smoothly locks up to direct gear ratio when engine load reaches 0 at steady cruise.

## 3. Verification & Testing

- [ ] 3.1 Add host unit test in `test/host_vc/` asserting torque converter RPM flaring on step throttle, launch slip boost, and lockup on speed stabilization.
- [ ] 3.2 Verify build and end-to-end execution on live hardware via BLE Remote API.
