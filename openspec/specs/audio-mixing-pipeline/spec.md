# Audio Mixing Pipeline Specification

## Purpose
Defines requirements for 16-bit block-based audio rendering, voice group gain staging, and dynamic volume scaling.
## Requirements
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

### Requirement: 4-Point Hermite cubic spline audio resampling
The sound engine SHALL perform fractional sample resampling using a 4-point, 3rd-order Hermite (Catmull-Rom) cubic spline polynomial to maintain smooth $C^1$ derivative continuity across sample boundaries during pitch shifting:
1. **Spline Formulation**: Given 4 neighbouring sample points $s_0, s_1, s_2, s_3$ and fractional offset $\alpha \in [0.0, 1.0)$, the interpolated sample value SHALL be computed using Horner's polynomial form:
   $$c_0 = s_1$$
   $$c_1 = 0.5 \cdot (s_2 - s_0)$$
   $$c_2 = s_0 - 2.5 \cdot s_1 + 2.0 \cdot s_2 - 0.5 \cdot s_3$$
   $$c_3 = 0.5 \cdot (s_3 - s_0) + 1.5 \cdot (s_1 - s_2)$$
   $$\text{sample}(\alpha) = ((c_3 \cdot \alpha + c_2) \cdot \alpha + c_1) \cdot \alpha + c_0$$
2. **Boundary Handling**:
   - For looping voices, sample lookups beyond start/end bounds SHALL wrap cyclically within the defined loop region.
   - For one-shot voices, sample lookups prior to index 0 or beyond index $(N - 1)$ SHALL clamp to the boundary sample.

#### Scenario: Pitch shifted voice rendering
- **WHEN** a voice is pitch-shifted with fractional playback step $\alpha$
- **THEN** sample values are interpolated across 4 adjacent samples with continuous curvature, eliminating high-frequency slope artifacts

#### Scenario: Loop boundary interpolation
- **WHEN** fractional playback position wraps around the loop boundary
- **THEN** sample lookups cleanly wrap around the loop points without clicking or buffer overruns

### Requirement: Floating-point voice gain precomputation
The sound engine mixer `renderBlock()` SHALL precompute combined floating-point gain coefficients for all active voices prior to the per-frame sample loop:
1. Combined gain factor $\text{gain}_i = (\text{vol}_i \cdot 0.01) \cdot \text{groupMultiplier}_i \cdot \text{mixWeight} \cdot \text{master} \cdot 256.0$.
2. Per-sample voice accumulation SHALL execute via single-cycle floating-point multiply-accumulate operations without per-frame integer divisions.

#### Scenario: Multi-voice block mixing efficiency
- **WHEN** `renderBlock()` is called for a 64-frame buffer
- **THEN** all voice gains are precomputed outside the frame loop and accumulated using hardware FPU floating-point operations

### Requirement: Warm analog soft-knee output limiting
The sound engine `renderBlock()` method SHALL apply an FPU-accelerated cubic polynomial soft-knee saturation curve to the mixed audio accumulator before casting to 16-bit signed PCM:
1. When normalized amplitude $|x| \le \frac{2}{3}$, the output SHALL be linear ($y = x$) with bit-exact preservation of original waveform dynamics.
2. When $\frac{2}{3} < |x| < 1.0$, the output SHALL follow the cubic saturation curve $y = \text{sgn}(x) \cdot \frac{3 - (2 - 3|x|)^2}{3}$, rounding off peak crests smoothly.
3. When $|x| \ge 1.0$, the output SHALL clamp smoothly to $\text{sgn}(x) \cdot 1.0$, eliminating hard-edge square wave clipping harmonics.

#### Scenario: Loud multi-voice audio peak
- **WHEN** multiple high-volume voices sum to an amplitude exceeding $\pm 32767$
- **THEN** the output saturates with a smooth continuous curve without sharp square-wave clipping distortion

### Requirement: Continuous floating-point flywheel inertia simulation
The sound engine simulation SHALL calculate virtual flywheel RPM acceleration, deceleration, and torque converter slip using continuous floating-point differential exponential smoothing:
1. `currentRpm` and `effectiveTarget` SHALL be computed with fractional sub-RPM precision.
2. Inertia integration SHALL be calculated via continuous exponential time decay $\Delta\text{RPM} = (\text{Target} - \text{RPM}) \cdot (1.0 - e^{-\Delta t / \tau})$.

#### Scenario: Idle RPM throttle transition
- **WHEN** throttle is feathered gently at low RPM
- **THEN** engine RPM increases smoothly with continuous fractional pitch transitions without integer division quantization steps

### Requirement: Non-blocking real-time audio task execution
The `audioTask` in `AudioOutput.h` SHALL execute without blocking I/O or direct serial logging calls inside the real-time sample generation and I2S DMA write loop:
1. The task SHALL NOT invoke `Serial.printf` or any blocking UART/USB CDC logging functions inside its main loop.
2. Block generation statistics SHALL be maintained in atomic counters accessible asynchronously for diagnostics.

#### Scenario: Audio rendering during disconnected or stalled USB serial
- **WHEN** USB serial TX buffer is full or disconnected
- **THEN** `audioTask` continues uninterrupted without DMA underruns or buffer starvation

### Requirement: Precomputed reciprocal invariants in sound synthesis
The sound engine `RcEngineSound` SHALL precalculate and cache reciprocal floating-point invariants during `setConfig()`:
1. Invariant terms including `invRpmRange = 1.0f / (maxRpm - idleEndPoint)` and `invMaxRpm = 1.0f / maxRpm` SHALL be computed at configuration time.
2. Pitch scaling and throttle percentage calculations in `update()` SHALL use precalculated reciprocals with floating-point multiplication instead of runtime division.

#### Scenario: Real-time pitch calculation
- **WHEN** engine RPM changes during throttle transitions
- **THEN** `pitchFactor` is computed using single-cycle floating-point multiplication with `invRpmRange`

