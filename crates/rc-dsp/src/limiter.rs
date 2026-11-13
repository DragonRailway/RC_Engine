//! Rational soft-knee saturator / limiter preventing 16-bit integer clipping.

#[cfg(not(feature = "std"))]
use libm::{fabsf, roundf};

#[cfg(feature = "std")]
#[inline]
fn fabsf(x: f32) -> f32 {
    x.abs()
}

#[cfg(feature = "std")]
#[inline]
fn roundf(x: f32) -> f32 {
    x.round()
}

/// Applies a rational soft-knee compression curve to `mix_accum`.
///
/// - For signals below 2/3 full scale (~ -3.5 dBFS), the transfer function is purely linear (y = x).
/// - For signals exceeding 2/3 full scale, an asymptotic rational curve smoothly limits peaks to 1.0.
/// - Returns signed 16-bit PCM in [-32768, 32767].
pub fn saturate_soft_knee(mix_accum: f32) -> i16 {
    let x = mix_accum / 32767.0;
    let ax = fabsf(x);
    const T: f32 = 2.0 / 3.0;
    const INV_RANGE: f32 = 1.0 / (1.0 - T); // 3.0

    let y = if ax <= T {
        x
    } else {
        let delta = ax - T;
        let sat = T + delta / (1.0 + delta * INV_RANGE);
        if x < 0.0 {
            -sat
        } else {
            sat
        }
    };

    let out_sample = roundf(y * 32767.0) as i32;
    out_sample.clamp(-32768, 32767) as i16
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_linear_region() {
        // Under 2/3 full scale (e.g. 10000.0), output should match input exactly
        let out = saturate_soft_knee(10000.0);
        assert_eq!(out, 10000);

        let out_neg = saturate_soft_knee(-10000.0);
        assert_eq!(out_neg, -10000);
    }

    #[test]
    fn test_saturation_region() {
        // High overload should smoothly compress and never wrap
        let out_extreme = saturate_soft_knee(150000.0);
        assert!(out_extreme > 30000);

        let out_extreme_neg = saturate_soft_knee(-150000.0);
        assert!(out_extreme_neg < -30000);
    }
}
