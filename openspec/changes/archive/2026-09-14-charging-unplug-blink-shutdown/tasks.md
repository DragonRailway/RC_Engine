## 1. Hardware Power-Off with Blink Implementation

- [x] 1.1 Add `HardwareInit::powerOffWithBlink(int8_t indicatorPin, uint8_t blinks = 4, uint16_t onMs = 100, uint16_t offMs = 100)` in `common/HardwareInit.h` and `common/HardwareInit.cpp`
- [x] 1.2 Update `VehicleController::update()` to resolve indicator pin cascade and call `HardwareInit::powerOffWithBlink()` upon USB unplug in charging mode

## 2. Verification and Tests

- [x] 2.1 Update `test/host_vc/host_vc_driver.cpp` to verify the unplug 4-blink sequence and safe power cut
- [x] 2.2 Run host tests (`python3 scripts/host_vc_test.py`) and build firmware across board environments (`MIKRO_V2`, `TRACKLINK_V2`, `TRACKLINK_V3`)
