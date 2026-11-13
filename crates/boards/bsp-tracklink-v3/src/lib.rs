//! Board Support Package for TrackLink V3.
//!
//! Dual H-bridge motor drivers, MAX98357A I2S DAC, power latch on GPIO 33,
//! and 6 low-side light outputs.

#![no_std]

use bsp_core::*;
use rc_drivers_esp32s3::BoardHardware;

pub const PIN_VOCABULARY: &[(&str, u8)] = &[
    ("L0", 42), // BUILTIN LED
    ("L1", 11),
    ("L2", 10),
    ("L3", 9),
    ("L4", 8),
    ("L5", 7),
    ("L6", 6),
    ("S1", 1),
    ("S2", 2),
];

pub struct TrackLinkV3;

impl TrackLinkV3 {
    /// Initialize the physical hardware structure for TrackLink V3.
    pub fn init_hardware() -> BoardHardware {
        BoardHardware::from_board::<Self>()
    }
}

impl Board for TrackLinkV3 {
    const NAME: &'static str = "TRACKLINK_V3";

    const VOLTAGE_DIVIDER: ResistorDivider = ResistorDivider::new(100.0, 100.0);

    const AUDIO: AudioPins = AudioPins {
        lrc: 17,
        bclk: 18,
        din: 21,
        sd: Some(47),
    };

    const POWER: PowerPins = PowerPins {
        power_enable: Some(33),
        power_button: Some(48),
        voltage_sens: Some(2),
        charge_sens: Some(34),
        servo_enable: None,
        pump_enable: None,
    };

    const DRIVER_A: DualPwmPins = DualPwmPins {
        pwm1: 13,
        pwm2: 14,
        enable: Some(12),
        bemf: Some(4),
        isens: None,
    };

    const DRIVER_B: Option<DualPwmPins> = Some(DualPwmPins {
        pwm1: 15,
        pwm2: 16,
        enable: Some(12),
        bemf: Some(5),
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
    fn test_tracklink_v3_pins() {
        assert_eq!(TrackLinkV3::NAME, "TRACKLINK_V3");
        assert_eq!(TrackLinkV3::resolve_pin("L1"), Some(11));
        assert_eq!(TrackLinkV3::resolve_pin("L6"), Some(6));
        assert_eq!(TrackLinkV3::resolve_pin("S1"), Some(1));
        assert_eq!(TrackLinkV3::resolve_pin("UNKNOWN"), None);

        let driver_a = TrackLinkV3::resolve_driver("DRIVER_A").unwrap();
        assert_eq!(driver_a.pwm1, 13);
        assert_eq!(driver_a.pwm2, 14);

        let hw = TrackLinkV3::init_hardware();
        assert_eq!(hw.name, "TRACKLINK_V3");
        assert_eq!(hw.power.power_latch_pin, Some(33));
    }
}
