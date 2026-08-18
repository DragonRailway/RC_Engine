## 1. Design & Metadata Verification

- [x] 1.1 Update `docs/radiokit-rc-ui-design.json` speed telemetry unit to `"km/h"`

## 2. Firmware Telemetry Implementation

- [x] 2.1 Update `common/VehicleController.h` `updateTelemetry()` to compute battery percentage with `s_warningVoltage` as 0% floor and clamp to [0, 100]
- [x] 2.2 Update `common/VehicleController.h` `updateTelemetry()` to compute speed in km/h (`abs(motorSpeed) * 2`) and update `telemetry_Speed.rk.content`

## 3. Host Tests & Verification

- [x] 3.1 Update `test/host_vc/host_vc_driver.cpp` to assert battery 0% at warning voltage and speed scaling in km/h
- [x] 3.2 Run `./test/host_vc/host_vc_harness` to verify all assertions pass

## 4. Build & Hardware Verification

- [x] 4.1 Compile firmware with `pio run -e MIKRO_V2`
- [x] 4.2 Flash firmware with `pio run -e MIKRO_V2 -t upload`
- [x] 4.3 Verify telemetry stream via live USB JTAG / RadioKit API
