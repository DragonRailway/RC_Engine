//! LittleFS partition configuration and asset loading abstractions.

#![no_std]

use heapless::String;
use rc_core::hardware_config::{parse_hardware_config, HardwareConfig};

/// Standard file paths in the vehicle partition.
pub mod paths {
    pub const HARDWARE_JSON: &str = "/config/hardware.json";
    pub const VEHICLE_JSON: &str = "/config/vehicle.json";
    pub const SOUNDS_DIR: &str = "/sounds";
}

/// In-memory or LittleFS file buffer descriptor.
#[derive(Debug, Clone, Copy)]
pub struct FlashAsset<'a> {
    pub path: &'a str,
    pub data: &'a [u8],
}

/// Vehicle configuration loader abstraction.
pub struct ConfigLoader;

impl ConfigLoader {
    /// Parse `/config/hardware.json` data into typed `HardwareConfig`.
    pub fn load_hardware<'a>(raw_json: &'a [u8]) -> Result<HardwareConfig<'a>, ()> {
        parse_hardware_config(raw_json).map_err(|_| ())
    }

    /// Construct full path for a sound file name in `/sounds/`.
    pub fn sound_path(name: &str) -> String<32> {
        let mut s = String::new();
        let _ = s.push_str(paths::SOUNDS_DIR);
        let _ = s.push('/');
        let _ = s.push_str(name);
        let _ = s.push_str(".pcm");
        s
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_sound_path_generation() {
        let path = ConfigLoader::sound_path("idle");
        assert_eq!(path.as_str(), "/sounds/idle.pcm");

        let horn = ConfigLoader::sound_path("horn");
        assert_eq!(horn.as_str(), "/sounds/horn.pcm");
    }

    #[test]
    fn test_config_loader_hardware() {
        let json = br#"{"drivetrain":{}}"#;
        let cfg = ConfigLoader::load_hardware(json);
        assert!(cfg.is_ok());
    }
}
