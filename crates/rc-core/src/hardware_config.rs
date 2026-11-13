//! Hardware configuration structures and zero-copy JSON parsing from LittleFS.

use bsp_core::{Board, DynamicPinPool, PinError};
use heapless::Vec;
use serde::Deserialize;

#[derive(Debug, Clone, Deserialize)]
pub struct MotorConfig<'a> {
    pub hardware: &'a str, // e.g. "DRIVER_A"
    #[serde(default = "default_freq")]
    pub frequency: u32,
    #[serde(default)]
    pub direction: Option<&'a str>, // "forward" | "reverse"
}

fn default_freq() -> u32 {
    20000
}

#[derive(Debug, Clone, Deserialize, Default)]
pub struct DrivetrainHardwareConfig<'a> {
    #[serde(borrow)]
    pub drive_motors: Option<Vec<MotorConfig<'a>, 2>>,
    #[serde(borrow)]
    pub left_motor: Option<MotorConfig<'a>>,
    #[serde(borrow)]
    pub right_motor: Option<MotorConfig<'a>>,
}

#[derive(Debug, Clone, Deserialize)]
pub struct LightChannelConfig<'a> {
    #[serde(borrow)]
    pub hardware: Vec<&'a str, 4>, // e.g. ["L1", "L2"]
    #[serde(default = "default_brightness")]
    pub brightness_max: u8,
}

fn default_brightness() -> u8 {
    100
}

#[derive(Debug, Clone, Deserialize, Default)]
pub struct LightsHardwareConfig<'a> {
    #[serde(borrow)]
    pub head_light: Option<LightChannelConfig<'a>>,
    #[serde(borrow)]
    pub tail_light: Option<LightChannelConfig<'a>>,
    #[serde(borrow)]
    pub brake_light: Option<LightChannelConfig<'a>>,
    #[serde(borrow)]
    pub cab_light: Option<LightChannelConfig<'a>>,
    #[serde(borrow)]
    pub step_light: Option<LightChannelConfig<'a>>,
}

#[derive(Debug, Clone, Deserialize)]
pub struct BatteryHardwareConfig {
    #[serde(default = "default_cell_count")]
    pub cell_count: u8,
    #[serde(default = "default_warning_v")]
    pub warning_voltage: f32,
    #[serde(default = "default_cutoff_v")]
    pub cutoff_voltage: f32,
}

fn default_cell_count() -> u8 {
    2
}
fn default_warning_v() -> f32 {
    3.4
}
fn default_cutoff_v() -> f32 {
    3.2
}

#[derive(Debug, Clone, Deserialize, Default)]
pub struct HardwareConfig<'a> {
    #[serde(borrow)]
    pub drivetrain: DrivetrainHardwareConfig<'a>,
    #[serde(borrow)]
    pub lights: Option<LightsHardwareConfig<'a>>,
    pub battery: Option<BatteryHardwareConfig>,
}

/// Parse a raw LittleFS JSON buffer into strongly typed `HardwareConfig`.
pub fn parse_hardware_config<'a>(json: &'a [u8]) -> Result<HardwareConfig<'a>, serde_json_core::de::Error> {
    let (cfg, _) = serde_json_core::from_slice(json)?;
    Ok(cfg)
}

/// Resolved runtime hardware bindings after matching against a Board's pin vocabulary.
#[derive(Debug, Default)]
pub struct BoundHardware {
    pub head_light_gpios: Vec<u8, 4>,
    pub tail_light_gpios: Vec<u8, 4>,
    pub brake_light_gpios: Vec<u8, 4>,
    pub cab_light_gpios: Vec<u8, 4>,
    pub step_light_gpios: Vec<u8, 4>,
}

impl BoundHardware {
    /// Bind LittleFS JSON config pins to physical GPIOs using the board's vocabulary
    /// and safely claim them from the dynamic pin pool.
    pub fn bind<B: Board>(
        config: &HardwareConfig,
        pin_pool: &mut DynamicPinPool,
    ) -> Result<Self, PinError> {
        let mut bound = Self::default();

        if let Some(ref lights) = config.lights {
            if let Some(ref hl) = lights.head_light {
                for &name in &hl.hardware {
                    if let Some(gpio) = B::resolve_pin(name) {
                        pin_pool.claim_pin(gpio)?;
                        let _ = bound.head_light_gpios.push(gpio);
                    }
                }
            }
            if let Some(ref tl) = lights.tail_light {
                for &name in &tl.hardware {
                    if let Some(gpio) = B::resolve_pin(name) {
                        pin_pool.claim_pin(gpio)?;
                        let _ = bound.tail_light_gpios.push(gpio);
                    }
                }
            }
            if let Some(ref bl) = lights.brake_light {
                for &name in &bl.hardware {
                    if let Some(gpio) = B::resolve_pin(name) {
                        pin_pool.claim_pin(gpio)?;
                        let _ = bound.brake_light_gpios.push(gpio);
                    }
                }
            }
            if let Some(ref cl) = lights.cab_light {
                for &name in &cl.hardware {
                    if let Some(gpio) = B::resolve_pin(name) {
                        pin_pool.claim_pin(gpio)?;
                        let _ = bound.cab_light_gpios.push(gpio);
                    }
                }
            }
            if let Some(ref sl) = lights.step_light {
                for &name in &sl.hardware {
                    if let Some(gpio) = B::resolve_pin(name) {
                        pin_pool.claim_pin(gpio)?;
                        let _ = bound.step_light_gpios.push(gpio);
                    }
                }
            }
        }

        Ok(bound)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    // Mock Board for testing
    struct MockTrackLinkV3;
    impl Board for MockTrackLinkV3 {
        const NAME: &'static str = "MOCK_TL3";
        const VOLTAGE_DIVIDER: bsp_core::ResistorDivider = bsp_core::ResistorDivider::new(100.0, 100.0);
        const AUDIO: bsp_core::AudioPins = bsp_core::AudioPins { lrc: 17, bclk: 18, din: 21, sd: None };
        const POWER: bsp_core::PowerPins = bsp_core::PowerPins {
            power_enable: Some(33),
            power_button: Some(48),
            voltage_sens: Some(2),
            charge_sens: None,
            servo_enable: None,
            pump_enable: None,
        };
        const DRIVER_A: bsp_core::DualPwmPins = bsp_core::DualPwmPins { pwm1: 13, pwm2: 14, enable: None, bemf: None, isens: None };
        const DRIVER_B: Option<bsp_core::DualPwmPins> = None;

        fn resolve_pin(name: &str) -> Option<u8> {
            match name {
                "L1" => Some(11),
                "L2" => Some(10),
                "L3" => Some(9),
                "L4" => Some(8),
                _ => None,
            }
        }
        fn pin_vocabulary() -> &'static [(&'static str, u8)] {
            &[("L1", 11), ("L2", 10), ("L3", 9), ("L4", 8)]
        }
    }

    #[test]
    fn test_parse_and_bind_hardware_json() {
        let json_data = br#"{
            "drivetrain": {
                "drive_motors": [
                    { "hardware": "DRIVER_A", "frequency": 20000, "direction": "forward" }
                ]
            },
            "lights": {
                "head_light": { "hardware": ["L1"], "brightness_max": 80 },
                "tail_light": { "hardware": ["L2"], "brightness_max": 60 }
            }
        }"#;

        let config = parse_hardware_config(json_data).expect("valid json parse");
        let mut pool = DynamicPinPool::new();

        let bound = BoundHardware::bind::<MockTrackLinkV3>(&config, &mut pool).expect("successful pin binding");
        assert_eq!(bound.head_light_gpios.as_slice(), &[11]);
        assert_eq!(bound.tail_light_gpios.as_slice(), &[10]);
        assert!(pool.is_claimed(11));
        assert!(pool.is_claimed(10));
        assert!(!pool.is_claimed(9)); // L3 not in JSON, not claimed

        // Conflicting configuration attempting to claim L1 twice must fail
        let mut dup_pool = DynamicPinPool::new();
        dup_pool.claim_pin(11).unwrap();
        let conflict = BoundHardware::bind::<MockTrackLinkV3>(&config, &mut dup_pool);
        assert_eq!(conflict.err(), Some(PinError::AlreadyClaimed(11)));
    }
}
