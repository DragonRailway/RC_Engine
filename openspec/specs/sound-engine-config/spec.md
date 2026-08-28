## ADDED Requirements

### Requirement: Hardware Sound Opt-in and Asset Gate
The firmware SHALL only initialize physical audio output hardware (`AudioOutput`) if the hardware configuration explicitly declares a `"sound"` block (`hwConfig.sound.configured == true`) AND at least one valid sound sample slot is successfully loaded into memory (`loadedCount > 0`).

#### Scenario: Sound enabled with valid assets and hardware config
- **WHEN** `hardware-<BOARD>.json` defines `"sound"` with a non-zero volume and the vehicle bundle contains at least one resolvable sound sample file
- **THEN** the firmware loads sound samples and initializes `AudioOutput` with I2S DMA streaming and audio task creation

#### Scenario: Sound hardware disabled when omitted from hardware config
- **WHEN** `hardware-<BOARD>.json` omits the `"sound"` block
- **THEN** the firmware bypasses sound asset loading and does not initialize `AudioOutput`, claiming zero audio pins, zero DMA buffers, and zero audio tasks

#### Scenario: Sound hardware disabled when no sound files are present
- **WHEN** `hardware-<BOARD>.json` defines `"sound"` but the vehicle bundle and fallbacks contain zero resolvable sound sample files
- **THEN** the firmware does not initialize `AudioOutput`, avoiding unnecessary I2S DMA and audio processing tasks

### Requirement: Vehicle Engine Simulation Independence
The vehicle physics and state simulation (`RcEngineSound` RPM calculation, flywheel inertia, transmission shifting, torque converter slip, and start/stop states) SHALL operate continuously regardless of whether audio output hardware is enabled or disabled.

#### Scenario: Physics simulation runs when sound is disabled
- **WHEN** sound hardware is disabled due to missing `"sound"` in hardware config or absence of sound files
- **THEN** `RcEngineSound` continues simulating engine state and RPM to drive throttle mapping and motor PWM in `VehicleController`
