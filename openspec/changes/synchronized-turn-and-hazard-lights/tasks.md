## 1. HardwareInit Synchronized Turn & Hazard Controller

- [x] 1.1 Add `HardwareInit::setTurnSignals(bool left, bool right, bool hazard, uint16_t onMs, uint16_t offMs, uint8_t brightness)` to `HardwareInit.h` and `HardwareInit.cpp`.
- [x] 1.2 Implement `TurnMode` state machine in `HardwareInit` with simultaneous timer reset on Hazard entry.

## 2. VehicleController Integration & Audio Sync

- [x] 2.1 Update `VehicleController::applyLightsWithAutomation()` to route through `HardwareInit::setTurnSignals()`.
- [x] 2.2 Synchronize the indicator audio click sound trigger with turn/hazard mode transitions.

## 3. Verification & Testing

- [x] 3.1 Update `host_vc_driver.cpp` with test assertions for turn-to-hazard synchronization.
- [x] 3.2 Run host test suites and build all firmware environments.
- [x] 3.3 Flash `MIKRO_V2` board and verify synchronized hazard flashing.
