//! 4-point Catmull-Rom Hermite cubic spline interpolation.
//!
//! Provides smooth, band-limited pitch shifting across arbitrary fractional sample positions
//! without audible aliasing or phase discontinuities at loop points.

/// Read an interpolated sample from an 8-bit signed PCM buffer using 4-point Hermite spline.
///
/// - `samples`: PCM buffer slice
/// - `position`: Current floating-point read head position
/// - `loop_mode`: Whether to loop the sample
/// - `loop_begin`: Loop start index (used if `loop_end > loop_begin`)
/// - `loop_end`: Loop end index
pub fn read_interpolated_hermite_4p(
    samples: &[i8],
    position: f32,
    loop_mode: bool,
    loop_begin: usize,
    loop_end: usize,
) -> f32 {
    let count = samples.len();
    if count == 0 {
        return 0.0;
    }
    if count == 1 {
        return samples[0] as f32;
    }

    let pos = position as i32;
    let mut alpha = position - (pos as f32);
    if alpha < 0.0 {
        alpha = 0.0;
    } else if alpha > 1.0 {
        alpha = 1.0;
    }

    let (i0, i1, i2, i3): (usize, usize, usize, usize) = if loop_mode && loop_end > loop_begin && loop_end <= count {
        let l_start = loop_begin as i32;
        let l_len = (loop_end - loop_begin) as i32;
        if pos < l_start {
            let p1 = if pos >= 0 { pos } else { 0 };
            let p0 = if p1 > 0 { p1 - 1 } else { 0 };
            let p2 = if p1 + 1 < count as i32 { p1 + 1 } else { p1 };
            let p3 = if p1 + 2 < count as i32 { p1 + 2 } else { p2 };
            (p0 as usize, p1 as usize, p2 as usize, p3 as usize)
        } else {
            let mut offset = (pos - l_start) % l_len;
            if offset < 0 {
                offset += l_len;
            }
            let p1 = l_start + offset;
            let p0 = l_start + ((offset - 1 + l_len) % l_len);
            let p2 = l_start + ((offset + 1) % l_len);
            let p3 = l_start + ((offset + 2) % l_len);
            (p0 as usize, p1 as usize, p2 as usize, p3 as usize)
        }
    } else if loop_mode {
        let c = count as i32;
        let p = (pos % c + c) % c;
        let p1 = p;
        let p0 = (p - 1 + c) % c;
        let p2 = (p + 1) % c;
        let p3 = (p + 2) % c;
        (p0 as usize, p1 as usize, p2 as usize, p3 as usize)
    } else {
        let max_idx = (count - 1) as i32;
        let p1 = if pos < 0 {
            0
        } else if pos > max_idx {
            max_idx
        } else {
            pos
        };
        let p0 = if p1 > 0 { p1 - 1 } else { 0 };
        let p2 = if p1 + 1 <= max_idx { p1 + 1 } else { max_idx };
        let p3 = if p1 + 2 <= max_idx { p1 + 2 } else { max_idx };
        (p0 as usize, p1 as usize, p2 as usize, p3 as usize)
    };

    let s0 = samples[i0] as f32;
    let s1 = samples[i1] as f32;
    let s2 = samples[i2] as f32;
    let s3 = samples[i3] as f32;

    let c0 = s1;
    let c1 = 0.5 * (s2 - s0);
    let c2 = s0 - 2.5 * s1 + 2.0 * s2 - 0.5 * s3;
    let c3 = 0.5 * (s3 - s0) + 1.5 * (s1 - s2);

    ((c3 * alpha + c2) * alpha + c1) * alpha + c0
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_exact_integer_position() {
        let buffer: [i8; 4] = [10, 20, 30, 40];
        let val = read_interpolated_hermite_4p(&buffer, 1.0, false, 0, 0);
        assert!((val - 20.0).abs() < 1e-4);
    }

    #[test]
    fn test_midpoint_interpolation() {
        let buffer: [i8; 4] = [0, 10, 20, 30];
        let val = read_interpolated_hermite_4p(&buffer, 1.5, false, 0, 0);
        // Linear midpoint between 10 and 20 is 15.0. Hermite on linear series is exact 15.0.
        assert!((val - 15.0).abs() < 1e-4);
    }

    #[test]
    fn test_loop_wrap_continuity() {
        let buffer: [i8; 4] = [10, 20, 30, 40];
        let val_at_end = read_interpolated_hermite_4p(&buffer, 3.99, true, 0, 0);
        let val_at_start = read_interpolated_hermite_4p(&buffer, 0.0, true, 0, 0);
        assert!((val_at_end - val_at_start).abs() < 5.0);
    }
}
