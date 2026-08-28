## MODIFIED Requirements

### Requirement: RadioKit library integration
The build SHALL obtain the RadioKit Arduino library directly from GitHub (`https://github.com/Radio-Kit/RadioKit.git` branch `main`, path `rk-arduino`) via a pre-build fetch script (`scripts/fetch_radiokit.py`) placed in `lib/rk-arduino`, so `RadioKitLib.h` and its widget classes are available to `src/` without requiring local sibling path dependencies.

#### Scenario: PlatformIO build with auto-fetched RadioKit
- **WHEN** running `pio run -e TRACKLINK_V3` or `pio run -e MIKRO_V2` on a clean checkout where `lib/rk-arduino` is absent
- **THEN** the pre-build script fetches `rk-arduino` from GitHub into `lib/rk-arduino` and the build succeeds

#### Scenario: RadioKit header is includable
- **WHEN** `src/RADIOKIT.h` includes `<RadioKitLib.h>`
- **THEN** the widget and library symbols (`RK_Knob`, `RK_GasPedal`, `RK_MultipleSelect`, `RK_Slider`, `RK_SlideSwitch`, `RK_PushButton`, `RK_Telemetry`, `RadioKit`) are available
