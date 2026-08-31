## ADDED Requirements

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
