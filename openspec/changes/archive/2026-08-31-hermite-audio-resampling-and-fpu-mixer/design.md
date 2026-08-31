## Context

`RcEngineSound::renderBlock` mixes up to 32 voice slots into a 64-sample stereo buffer at 22,050 Hz. Currently, every frame performs fractional step interpolation using 2 linear points and applies voice volume, group weight, engine mix weight, and master volume via consecutive integer multiplications and divisions.

The ESP32-S3 features an Xtensa LX7 dual-core processor with a pipelined single-precision Floating-Point Unit (FPU). Floating-point multiplications (`fmul.s`) and fused multiply-add (`madd.s`) execute in a single clock cycle, whereas integer software divisions take 12–30 clock cycles.

## Goals / Non-Goals

**Goals:**
- Implement 4-point, 3rd-order Hermite (Catmull-Rom) cubic spline interpolation for all active voices with seamless loop boundary wrapping.
- Precompute combined float gain factors outside the per-sample loop in `renderBlock()`.
- Reduce block rendering execution time and eliminate high-frequency aliasing artifacts at high pitch factors.

**Non-Goals:**
- Changing sample rate (stays 22,050 Hz) or PCM bit depth (8-bit PCM stored, 16-bit stereo output).
- Altering the non-audio control loops or UI layer.

## Decisions

### 1. Hermite (Catmull-Rom) Spline Formula
- **Decision**: Evaluate the 4-point spline using Horner's polynomial form:
  $$c_0 = s_1$$
  $$c_1 = 0.5 \cdot (s_2 - s_0)$$
  $$c_2 = s_0 - 2.5 \cdot s_1 + 2.0 \cdot s_2 - 0.5 \cdot s_3$$
  $$c_3 = 0.5 \cdot (s_3 - s_0) + 1.5 \cdot (s_1 - s_2)$$
  $$\text{sample}(\alpha) = ((c_3 \cdot \alpha + c_2) \cdot \alpha + c_1) \cdot \alpha + c_0$$
- **Rationale**: Horner's form requires only 3 multiplies and 4 additions on the FPU, matching the performance of linear interpolation while guaranteeing smooth continuous first derivatives ($C^1$) across sample boundaries.

### 2. Loop Boundary Index Wrapping
- **Decision**: For looped voices, wrap boundary index lookups `[i0, i1, i2, i3]` around the loop boundaries `[loopBegin, loopEnd]`. For one-shot voices, clamp indices to `[0, count - 1]`.
- **Rationale**: Prevents clicks, phase discontinuities, or memory overruns when the playhead approaches the end of a loop or audio sample buffer.

### 3. Precomputed Float Coefficients
- **Decision**: Calculate `voiceGain[i] = (v.volume * 0.01f) * groupScale * mixWeight * masterScale * 256.0f` once per buffer block before the sample frames loop.
- **Rationale**: Collapses 4 separate scaling stages into a single float multiply-accumulate per active voice sample.

## Risks / Trade-offs

- [Risk] Float-to-integer conversion at output might clip if gain accumulation exceeds 16-bit range.
  - *Mitigation*: Clamp the accumulated float output between `-32768.0f` and `32767.0f` before casting to `int16_t`.
