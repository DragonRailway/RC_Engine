## 1. Board Foundation & Types

- [x] 1.1 Create `boards/BoardTypes.h` defining `PinEntry`, `DriverPins`, and `Driver::DualPWM` / `Driver::PwmDir` designated initializer helpers
- [x] 1.2 Create `boards/BoardBase.h` with `BaseBoard` struct providing default empty arrays (`PINS[]`, `DRIVERS[]`) and disabled peripheral structs (`AUDIO`, `POWER`)

## 2. Refactor Board Headers

- [x] 2.1 Refactor `boards/TRACKLINK_V3.h` to inherit `BaseBoard` with self-contained `PINS[]`, `DRIVERS[]`, `AUDIO`, and `POWER`
- [x] 2.2 Refactor `boards/MIKRO_V2.h` to inherit `BaseBoard` with self-contained `PINS[]`, `DRIVERS[]`, `AUDIO`, and `POWER`
- [x] 2.3 Refactor `boards/GTRACK.h` to inherit `BaseBoard` with self-contained `PINS[]`, `DRIVERS[]`, `AUDIO`, and `POWER`
- [x] 2.4 Update `boards/boards.h` to expose the unified `Board` API (`resolve`, `getDriver`, `hasAudio`, `hasDrivers`, `hasPowerLatch`) and update `scripts/gen_boards_header.py`

## 3. Decouple Common Infrastructure & PinMapper

- [x] 3.1 Refactor `common/PinMapper.h` to delegate resolution and driver lookup directly to `Board`, eliminating all board `#ifdef` macros
- [x] 3.2 Verify `common/HardwareInit.cpp` and `common/ConfigParser.cpp` compile seamlessly against the unified `Board` API

## 4. Verification & Testing

- [x] 4.1 Run PlatformIO build verification across all board environments (`TRACKLINK_V3`, `MIKRO_V2`, `GTRACK`)
- [x] 4.2 Run host tests (`pio test`) to ensure pin resolution and vehicle control logic remain 100% regression-free
