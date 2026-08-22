## ADDED Requirements

### Requirement: 16-Bit Block-Based Audio Mixing
The sound engine SHALL provide a block-based audio rendering method `renderBlock(int16_t* buffer, size_t frames)` that renders native 16-bit signed PCM samples directly into the I2S DMA buffer, eliminating the intermediate 8-bit quantization clamp and DC bias round-trip.

#### Scenario: Block rendering generates clean 16-bit signed PCM
- **WHEN** `AudioOutput` requests 64 frames of audio via `renderBlock(buffer, 64)`
- **THEN** `RcEngineSound` populates `buffer` with interleaved 16-bit signed PCM samples in the range [-32768, 32767] with 0 representing true silence.

#### Scenario: Zero output during engine OFF and stopped state
- **WHEN** the engine is in `OFF` state and no effect voices are active
- **THEN** `renderBlock()` fills the buffer with exact digital zeroes (silence) without DC bias.

### Requirement: Designated Voice Metadata Alignment
The sound engine voice definitions SHALL map to `SoundID` enum indices using C++ designated initializers to prevent array index misalignment.

#### Scenario: Jake brake voice properties
- **WHEN** `JAKE_BRAKE` sound is initialized in `RcEngineSound`
- **THEN** it SHALL be configured as `pitchShifted = true`, `loop = true`, `oneShot = false`.

#### Scenario: Cooling fan voice properties
- **WHEN** `FAN` sound is initialized in `RcEngineSound`
- **THEN** it SHALL be configured as `pitchShifted = true`, `loop = true`, `oneShot = false`.

#### Scenario: Siren voice properties
- **WHEN** `SIREN` sound is initialized in `RcEngineSound`
- **THEN** it SHALL be configured as `pitchShifted = false`, `loop = true`, `oneShot = false`.

#### Scenario: Air brake voice properties
- **WHEN** `BRAKE` sound is initialized in `RcEngineSound`
- **THEN** it SHALL be configured as `pitchShifted = false`, `loop = false`, `oneShot = true`.

### Requirement: Reference-Accurate Gain Staging
The sound engine mixer SHALL apply attenuation scaling factors corresponding to the reference project architecture:
- Engine voice group (Idle, Rev, Jake Brake, Start): $\times 0.8$
- Engine auxiliary group (Turbo, Fan, Supercharger): $\times 0.2$
- Ancillary / Triggered effects group B (Knock, Wastegate, Air Brake, Parking Brake, Shifting, Reversing, Coupling, Uncoupling): $\times 0.2$
- Horn / Siren: $\times 0.8$
- Excavator & Additional groups (Hydraulic Flow, Track Rattle, Bucket Rattle, Tire Squeal): $\times 1.0$

#### Scenario: Diesel knock gain scaling
- **WHEN** diesel knock is triggered with volume set to 400% in vehicle configuration
- **THEN** the effective mixed amplitude before master volume is scaled by $\times 0.2$ ($400\% \times 0.2 = 80\%$), preventing mixer saturation.

#### Scenario: Turbo whistle gain scaling
- **WHEN** turbo sound is active with volume set to 60% in vehicle configuration
- **THEN** the effective mixed amplitude is scaled by $\times 0.2$ ($60\% \times 0.2 = 12\%$), matching reference balance with engine idle.

### Requirement: Dynamic Throttle-Dependent Engine Volume
The engine idle and rev sound volumes SHALL scale dynamically with throttle according to `map(throttlePercent, 0, 100, idleVol, fullThrottleVol)`.

#### Scenario: Full throttle volume expansion
- **WHEN** throttle increases from 0% to 100%
- **THEN** engine sound volume expands from base idle volume up to configured `fullThrottle` percentage.
