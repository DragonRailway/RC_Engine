//! Procedural diesel engine knock pulse synthesis.
//!
//! Synthesizes the characteristic firing rhythm and uneven exhaust pulses of
//! various multi-cylinder engine configurations (V8, Chevy 468, Inline-6, Harley V2).

/// Engine cylinder firing order knock patterns.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum KnockPattern {
    #[default]
    V8,       // Ford / Scania V8: loud at 4, 8 of 8
    V8_468,   // Chevy 468: loud at 1, 5, 9, 13 of 16
    R6,       // Inline 6: loud at 6 of 6
    R6_2,     // Inline 6 alt: loud at 3, 6 of 6
    V2,       // Harley V2: loud at 1, 2 of 4
    Uniform,  // All cylinders fire with equal amplitude
}

impl KnockPattern {
    /// Return the total number of firing pulses per full engine cycle.
    pub const fn cycle_steps(&self) -> u8 {
        match self {
            KnockPattern::V8 => 8,
            KnockPattern::V8_468 => 16,
            KnockPattern::R6 | KnockPattern::R6_2 => 6,
            KnockPattern::V2 => 4,
            KnockPattern::Uniform => 1,
        }
    }

    /// Check if the cylinder at 1-based index `cylinder` produces a dominant/loud pulse.
    pub const fn is_loud_cylinder(&self, cylinder: u8) -> bool {
        match self {
            KnockPattern::V8 => cylinder == 4 || cylinder == 8,
            KnockPattern::V8_468 => cylinder == 1 || cylinder == 5 || cylinder == 9 || cylinder == 13,
            KnockPattern::R6 => cylinder == 6,
            KnockPattern::R6_2 => cylinder == 3 || cylinder == 6,
            KnockPattern::V2 => cylinder == 1 || cylinder == 2,
            KnockPattern::Uniform => true,
        }
    }
}

/// Knock generator configuration and runtime pulse tracker.
#[derive(Debug, Clone)]
pub struct KnockTracker {
    pub pattern: KnockPattern,
    pub cur_cylinder: u8,
    pub last_trigger_sample: u32,
    pub interval_samples: u32,
}

impl Default for KnockTracker {
    fn default() -> Self {
        Self::new(KnockPattern::V8)
    }
}

impl KnockTracker {
    pub const fn new(pattern: KnockPattern) -> Self {
        Self {
            pattern,
            cur_cylinder: 1,
            last_trigger_sample: 0,
            interval_samples: 0,
        }
    }

    /// Reset state for engine start.
    pub fn reset(&mut self) {
        self.cur_cylinder = 1;
        self.last_trigger_sample = 0;
        self.interval_samples = 0;
    }

    /// Advance cylinder step and return `(is_loud, cylinder_number)`.
    pub fn advance_step(&mut self) -> (bool, u8) {
        let max_steps = self.pattern.cycle_steps();
        let is_loud = self.pattern.is_loud_cylinder(self.cur_cylinder);
        let cyl = self.cur_cylinder;

        self.cur_cylinder += 1;
        if self.cur_cylinder > max_steps {
            self.cur_cylinder = 1;
        }

        (is_loud, cyl)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_v8_pattern_rhythm() {
        let mut tracker = KnockTracker::new(KnockPattern::V8);
        assert_eq!(tracker.pattern.cycle_steps(), 8);

        let mut loud_cylinders = [false; 8];
        for i in 0..8 {
            let (is_loud, cyl) = tracker.advance_step();
            assert_eq!(cyl, (i + 1) as u8);
            loud_cylinders[i] = is_loud;
        }

        // V8 is loud only at positions 4 and 8
        assert_eq!(loud_cylinders, [false, false, false, true, false, false, false, true]);
    }

    #[test]
    fn test_r6_pattern_rhythm() {
        let mut tracker = KnockTracker::new(KnockPattern::R6);
        assert_eq!(tracker.pattern.cycle_steps(), 6);

        let mut loud_cylinders = [false; 6];
        for i in 0..6 {
            let (is_loud, cyl) = tracker.advance_step();
            assert_eq!(cyl, (i + 1) as u8);
            loud_cylinders[i] = is_loud;
        }

        // R6 is loud at cylinder 6
        assert_eq!(loud_cylinders, [false, false, false, false, false, true]);
    }
}
