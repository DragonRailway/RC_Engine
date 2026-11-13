//! Core traits, pin pool, and shared hardware definitions for RC Engine BSPs.

#![no_std]

/// Resistor voltage divider configuration for battery ADC calculations.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct ResistorDivider {
    pub r_high_k: f32,
    pub r_low_k: f32,
}

impl ResistorDivider {
    pub const fn new(r_high_k: f32, r_low_k: f32) -> Self {
        Self { r_high_k, r_low_k }
    }

    /// Calculate the attenuation scaling factor (e.g. 2.0 for 100k / 100k).
    #[inline]
    pub fn ratio(&self) -> f32 {
        if self.r_low_k > 0.0 {
            (self.r_high_k + self.r_low_k) / self.r_low_k
        } else {
            1.0
        }
    }

    /// Convert raw ADC voltage reading at the pin to battery terminal voltage.
    #[inline]
    pub fn adc_to_battery_volts(&self, adc_volts: f32) -> f32 {
        adc_volts * self.ratio()
    }
}

/// Dual-PWM H-Bridge motor driver pin assignment.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct DualPwmPins {
    pub pwm1: u8,
    pub pwm2: u8,
    pub enable: Option<u8>,
    pub bemf: Option<u8>,
    pub isens: Option<u8>,
}

/// I2S audio DAC output pin assignment.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct AudioPins {
    pub lrc: u8,
    pub bclk: u8,
    pub din: u8,
    pub sd: Option<u8>, // Shutdown / Mute pin
}

/// Power latch, battery ADC, and power button pin assignment.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct PowerPins {
    pub power_enable: Option<u8>,
    pub power_button: Option<u8>,
    pub voltage_sens: Option<u8>,
    pub charge_sens: Option<u8>,
    pub servo_enable: Option<u8>,
    pub pump_enable: Option<u8>,
}

/// Error type when claiming a GPIO pin from the pool.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PinError {
    AlreadyClaimed(u8),
    InvalidPin(u8),
    NotFound,
}

/// Dynamic pin allocation pool tracking claimed GPIOs to prevent conflicting roles.
#[derive(Debug, Clone, Copy, Default)]
pub struct DynamicPinPool {
    claimed_mask: u64,
}

impl DynamicPinPool {
    pub const fn new() -> Self {
        Self { claimed_mask: 0 }
    }

    /// Check if a GPIO is currently claimed.
    #[inline]
    pub fn is_claimed(&self, gpio: u8) -> bool {
        if gpio >= 64 {
            return true;
        }
        (self.claimed_mask & (1u64 << gpio)) != 0
    }

    /// Claim a GPIO pin for a dedicated role. Returns an error if already claimed.
    pub fn claim_pin(&mut self, gpio: u8) -> Result<u8, PinError> {
        if gpio >= 49 {
            // ESP32-S3 valid GPIOs are 0..=48
            return Err(PinError::InvalidPin(gpio));
        }
        let mask = 1u64 << gpio;
        if (self.claimed_mask & mask) != 0 {
            return Err(PinError::AlreadyClaimed(gpio));
        }
        self.claimed_mask |= mask;
        Ok(gpio)
    }

    /// Release a previously claimed GPIO pin.
    pub fn release_pin(&mut self, gpio: u8) {
        if gpio < 64 {
            self.claimed_mask &= !(1u64 << gpio);
        }
    }
}

/// Common trait implemented by every Board Support Package.
pub trait Board {
    /// Canonical name of the board (e.g. "TRACKLINK_V3", "RK_ULTRA").
    const NAME: &'static str;

    /// Resistor divider configuration for battery voltage sense.
    const VOLTAGE_DIVIDER: ResistorDivider;

    /// I2S DAC pins for audio synthesis.
    const AUDIO: AudioPins;

    /// Power management and battery sense pins.
    const POWER: PowerPins;

    /// Primary motor driver (Driver A).
    const DRIVER_A: DualPwmPins;

    /// Secondary motor driver (Driver B), if equipped.
    const DRIVER_B: Option<DualPwmPins>;

    /// Resolve a logical pin name (e.g. "L1", "S1", "E1", "A1") to an MCU GPIO number.
    fn resolve_pin(name: &str) -> Option<u8>;

    /// Resolve a driver identifier (e.g. "DRIVER_A", "DRIVER_B") to physical driver pins.
    fn resolve_driver(name: &str) -> Option<DualPwmPins> {
        if name == "DRIVER_A" {
            Some(Self::DRIVER_A)
        } else if name == "DRIVER_B" {
            Self::DRIVER_B
        } else {
            None
        }
    }

    /// Returns the complete logical pin vocabulary of the board.
    fn pin_vocabulary() -> &'static [(&'static str, u8)];
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_resistor_divider_ratio() {
        let div_100k = ResistorDivider::new(100.0, 100.0);
        assert!((div_100k.ratio() - 2.0).abs() < 1e-4);
        assert!((div_100k.adc_to_battery_volts(3.7) - 7.4).abs() < 1e-4);

        let div_75k_10k = ResistorDivider::new(75.0, 10.0);
        assert!((div_75k_10k.ratio() - 8.5).abs() < 1e-4);
    }

    #[test]
    fn test_dynamic_pin_pool_single_claim() {
        let mut pool = DynamicPinPool::new();
        assert!(!pool.is_claimed(11));

        assert_eq!(pool.claim_pin(11), Ok(11));
        assert!(pool.is_claimed(11));

        // Claiming the same pin again must fail
        assert_eq!(pool.claim_pin(11), Err(PinError::AlreadyClaimed(11)));

        // Invalid pin > 48
        assert_eq!(pool.claim_pin(50), Err(PinError::InvalidPin(50)));

        // Releasing pin allows re-claiming
        pool.release_pin(11);
        assert!(!pool.is_claimed(11));
        assert_eq!(pool.claim_pin(11), Ok(11));
    }
}
