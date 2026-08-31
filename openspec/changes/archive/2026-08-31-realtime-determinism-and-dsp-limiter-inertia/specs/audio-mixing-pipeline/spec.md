## ADDED Requirements

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
