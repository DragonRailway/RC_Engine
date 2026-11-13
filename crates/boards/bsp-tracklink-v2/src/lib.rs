//! Board Support Package for TrackLink V2.
//!
//! Legacy locomotive control board featuring single H-bridge motor driver,
//! MAX98357A I2S DAC, and 6 LED channels.

#![no_std]

use bsp_core::*;

pub const PIN_VOCABULARY: &[(&str, u8)] = &[
    ("L0", 42), // BUILTIN LED
    ("L1", 9),
    ("L2", 10),
    ("L3", 11),
    ("L4", 12),
    ("L5", 13),
    ("L6", 14),
    ("S1", 1),
    ("S2", 2),
];

pub struct TrackLinkV2;

impl Board for TrackLinkV2 {
    const NAME: &'static str = "TRACKLINK_V2";

    const VOLTAGE_DIVIDER: ResistorDivider = ResistorDivider::new(100.0, 100.0);

    const AUDIO: AudioPins = AudioPins {
        lrc: 15,
        bclk: 16,
        din: 17,
        sd: Some(18),
    };

    const POWER: PowerPins = PowerPins {
        power_enable: Some(21),
        power_button: Some(38),
        voltage_sens: Some(4),
        charge_sens: Some(48),
        servo_enable: None,
        pump_enable: None,
    };

    const DRIVER_A: DualPwmPins = DualPwmPins {
        pwm1: 6,
        pwm2: 7,
        enable: Some(5),
        bemf: Some(8),
        isens: None,
    };

    const DRIVER_B: Option<DualPwmPins> = None;

    fn resolve_pin(name: &str) -> Option<u8> {
        for &(entry_name, pin) in PIN_VOCABULARY {
            if entry_name == name {
                return Some(pin);
            }
        }
        None
    }

    fn pin_vocabulary() -> &'static [(&'static str, u8)] {
        PIN_VOCABULARY
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_tracklink_v2_pins() {
        assert_eq!(TrackLinkV2::NAME, "TRACKLINK_V2");
        assert_eq!(TrackLinkV2::resolve_pin("L1"), Some(9));
        assert_eq!(TrackLinkV2::resolve_pin("L6"), Some(14));
        assert_eq!(TrackLinkV2::DRIVER_B, None);
    }
}
