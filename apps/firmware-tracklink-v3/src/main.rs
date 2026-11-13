//! Dedicated firmware application for TrackLink V3.

use bsp_core::{Board, DynamicPinPool};
use bsp_tracklink_v3::TrackLinkV3;
use rc_brain::*;
use rc_core::{hardware_config::*, VehicleType};

fn main() {
    println!("=== Booting RC Brain on {} ===", TrackLinkV3::NAME);
    println!("Power Latch Pin: GPIO {:?}", TrackLinkV3::POWER.power_enable);
    println!("Battery Voltage Divider Ratio: {:.2}", TrackLinkV3::VOLTAGE_DIVIDER.ratio());
    println!("Audio I2S DAC DIN Pin: GPIO {:?}", TrackLinkV3::AUDIO.din);

    // 1. Simulate reading LittleFS hardware.json config
    let sample_config_json = br#"{
        "drivetrain": {
            "drive_motors": [
                { "hardware": "DRIVER_A", "frequency": 20000, "direction": "forward" },
                { "hardware": "DRIVER_B", "frequency": 20000, "direction": "forward" }
            ]
        },
        "lights": {
            "head_light": { "hardware": ["L1"], "brightness_max": 80 },
            "tail_light": { "hardware": ["L2"], "brightness_max": 60 }
        }
    }"#;

    let config = parse_hardware_config(sample_config_json).expect("valid config");
    let mut pin_pool = DynamicPinPool::new();

    // 2. Bind hardware pins safely through TrackLinkV3 BSP
    let bound = BoundHardware::bind::<TrackLinkV3>(&config, &mut pin_pool)
        .expect("Hardware pin binding succeeded");

    println!("Bound Headlight GPIOs: {:?}", bound.head_light_gpios.as_slice());
    println!("Bound Taillight GPIOs: {:?}", bound.tail_light_gpios.as_slice());

    // 3. Initialize vehicle controller
    let mut vehicle = VehicleController::new(VehicleType::Locomotive, rc_core::DrivetrainMode::Ackermann);
    vehicle.battery.warning_voltage_per_cell = 3.4;
    vehicle.battery.cutoff_voltage_per_cell = 3.2;

    println!(
        "TrackLink V3 firmware initialized successfully: engine={:?}, cutoff={:.1}V",
        vehicle.engine.state,
        vehicle.battery.cutoff_voltage_per_cell
    );
}
