//! Board Support Package for Mikro V2.
//!
//! Designed for 1/24–1/35 scale micro trucks and crawlers. Dual motor drivers
//! with independent enable pins, MAX98357A audio, and 8 light channels.

#![no_std]

use bsp_core::*;

pub const PIN_VOCABULARY: &[(&str, u8)] = &[
    ("L0", 36), // Builtin LED
    ("L1", 38),
    ("L2", 39),
    ("L3", 40),
    ("L4", 41),
    ("L5", 42),
    ("L6", 43),
    ("L7", 1),
    ("L8", 2),
    ("S1", 5),
    ("S2", 6),
    ("S3", 7),
    ("S4", 8),
];

pub struct MikroV2;

impl Board for MikroV2 {
    const NAME: &'static str = "MIKRO_V2";

    const VOLTAGE_DIVIDER: ResistorDivider = ResistorDivider::new(100.0, 100.0);

    const AUDIO: AudioPins = AudioPins {
        lrc: 48,
        bclk: 47,
        din: 33,
        sd: Some(34),
    };

    const POWER: PowerPins = PowerPins {
        power_enable: Some(15),
        power_button: Some(14),
        voltage_sens: Some(4),
        charge_sens: Some(35),
        servo_enable: None,
        pump_enable: None,
    };

    const DRIVER_A: DualPwmPins = DualPwmPins {
        pwm1: 18,
        pwm2: 21,
        enable: Some(17),
        bemf: Some(9),
        isens: None,
    };

    const DRIVER_B: Option<DualPwmPins> = Some(DualPwmPins {
        pwm1: 12,
        pwm2: 13,
        enable: Some(11),
        bemf: Some(10),
        isens: None,
    });

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
    fn test_mikro_v2_pins() {
        assert_eq!(MikroV2::NAME, "MIKRO_V2");
        assert_eq!(MikroV2::resolve_pin("L1"), Some(38));
        assert_eq!(MikroV2::resolve_pin("L8"), Some(2));
        assert_eq!(MikroV2::resolve_pin("S1"), Some(5));
        assert_eq!(MikroV2::resolve_pin("S4"), Some(8));
        assert_eq!(MikroV2::resolve_pin("S5"), None);
    }
}
