//! Board Support Package for GTrack Dual-Track Excavators.
//!
//! TMI8260 H-bridge drivers, MAX98357A I2S DAC, 5V buck power enable,
//! external pump high-side switch, 8 servo outputs, and 6 LED channels.

#![no_std]

use bsp_core::*;
use rc_drivers_esp32s3::BoardHardware;

pub const PIN_VOCABULARY: &[(&str, u8)] = &[
    // Low-side MOSFET LEDs
    ("L1", 47),
    ("L2", 48),
    ("L3", 33),
    ("L4", 34),
    ("L5", 37),
    ("L6", 38),
    // Servos
    ("S1", 8),
    ("S2", 9),
    ("S3", 10),
    ("S4", 11),
    // Expansion Servos
    ("E1", 12),
    ("E2", 13),
    ("E3", 14),
    ("E4", 15),
    // Extra GPIOs
    ("A1", 1),
    ("A2", 2),
    ("A3", 3),
    ("A4", 4),
];

pub struct GTrack;

impl GTrack {
    /// Initialize the physical hardware structure for GTrack.
    pub fn init_hardware() -> BoardHardware {
        BoardHardware::from_board::<Self>()
    }
}

impl Board for GTrack {
    const NAME: &'static str = "GTRACK";

    // 20k / 5.1k divider -> Ratio = (20 + 5.1)/5.1 = ~4.9216
    const VOLTAGE_DIVIDER: ResistorDivider = ResistorDivider::new(20.0, 5.1);

    const AUDIO: AudioPins = AudioPins {
        lrc: 16,
        bclk: 17,
        din: 18,
        sd: Some(21),
    };

    const POWER: PowerPins = PowerPins {
        power_enable: Some(35),
        power_button: Some(36),
        voltage_sens: Some(7),
        charge_sens: None,
        servo_enable: Some(44), // Pull LOW to disable 5V buck
        pump_enable: Some(43),  // High-side switch for hydraulic pump / smoke unit
    };

    const DRIVER_A: DualPwmPins = DualPwmPins {
        pwm1: 39,
        pwm2: 40,
        enable: None,
        bemf: Some(5),
        isens: None,
    };

    const DRIVER_B: Option<DualPwmPins> = Some(DualPwmPins {
        pwm1: 41,
        pwm2: 42,
        enable: None,
        bemf: Some(6),
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
    fn test_gtrack_pins() {
        assert_eq!(GTrack::NAME, "GTRACK");
        assert_eq!(GTrack::resolve_pin("L1"), Some(47));
        assert_eq!(GTrack::resolve_pin("S1"), Some(8));
        assert_eq!(GTrack::resolve_pin("E1"), Some(12));
        assert_eq!(GTrack::resolve_pin("A1"), Some(1));

        let ratio = GTrack::VOLTAGE_DIVIDER.ratio();
        assert!((ratio - 4.9215686).abs() < 1e-4);

        let hw = GTrack::init_hardware();
        assert_eq!(hw.name, "GTRACK");
        assert_eq!(hw.power.power_latch_pin, Some(35));
    }
}
