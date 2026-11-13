//! Board Support Package for RK_ULTRA Flagship Control Board.
//!
//! Dual DRV8231A motor drivers with current sensing, MAX98357A I2S DAC,
//! LSM6DSVTR IMU (SPI), AW9523 I2C LED expander, and 8 servo channels.

#![no_std]

use bsp_core::*;

pub const PIN_VOCABULARY: &[(&str, u8)] = &[
    // 5V Servo Rails
    ("S1", 8),
    ("S2", 9),
    ("S3", 10),
    ("S4", 11),
    // Expansion Servos / PWM
    ("E1", 14),
    ("E2", 41),
    ("E3", 40),
    ("E4", 39),
    // Logic GPIOs
    ("A1", 42),
    ("A2", 43),
    ("A3", 44),
    // I2C Bus
    ("SDA", 37),
    ("SCL", 38),
];

pub struct RkUltra;

impl Board for RkUltra {
    const NAME: &'static str = "RK_ULTRA";

    // R13 = 75kΩ, R14 = 10kΩ -> Ratio = 8.5
    const VOLTAGE_DIVIDER: ResistorDivider = ResistorDivider::new(75.0, 10.0);

    const AUDIO: AudioPins = AudioPins {
        lrc: 15,
        bclk: 16,
        din: 17,
        sd: Some(18),
    };

    const POWER: PowerPins = PowerPins {
        power_enable: Some(35),
        power_button: Some(36),
        voltage_sens: Some(1),
        charge_sens: None,
        servo_enable: None,
        pump_enable: Some(48), // High-side MOSFET for external accessories
    };

    const DRIVER_A: DualPwmPins = DualPwmPins {
        pwm1: 2,
        pwm2: 3,
        enable: None, // Hardware VREF tied to 3.3V
        bemf: Some(5),
        isens: Some(4),
    };

    const DRIVER_B: Option<DualPwmPins> = Some(DualPwmPins {
        pwm1: 12,
        pwm2: 13,
        enable: None,
        bemf: Some(7),
        isens: Some(6),
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

/// Onboard LSM6DSVTR IMU SPI pins.
pub struct ImuPins;
impl ImuPins {
    pub const INT1: u8 = 21;
    pub const SDO: u8 = 26; // MISO
    pub const SDI: u8 = 47; // MOSI
    pub const CLK: u8 = 33; // SCK
    pub const CS: u8 = 34;  // CS
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_rk_ultra_pins() {
        assert_eq!(RkUltra::NAME, "RK_ULTRA");
        assert_eq!(RkUltra::resolve_pin("S1"), Some(8));
        assert_eq!(RkUltra::resolve_pin("E1"), Some(14));
        assert_eq!(RkUltra::resolve_pin("A1"), Some(42));
        assert_eq!(RkUltra::resolve_pin("SDA"), Some(37));

        // Voltage divider ratio for 75k / 10k is 8.5
        assert!((RkUltra::VOLTAGE_DIVIDER.ratio() - 8.5).abs() < 1e-4);

        let driver_a = RkUltra::resolve_driver("DRIVER_A").unwrap();
        assert_eq!(driver_a.isens, Some(4));
    }
}
