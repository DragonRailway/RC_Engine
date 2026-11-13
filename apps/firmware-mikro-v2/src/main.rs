//! Dedicated firmware application for Mikro V2.

use bsp_core::{Board, DynamicPinPool};
use bsp_mikro_v2::MikroV2;
use rc_brain::*;
use rc_core::{hardware_config::*, VehicleType};

fn main() {
    println!("=== Booting RC Brain on {} ===", MikroV2::NAME);
    println!("Power Latch Pin: GPIO {:?}", MikroV2::POWER.power_enable);
    println!("Battery Voltage Divider Ratio: {:.2}", MikroV2::VOLTAGE_DIVIDER.ratio());

    let sample_config_json = br#"{
        "drivetrain": {
            "drive_motors": [
                { "hardware": "DRIVER_A", "frequency": 20000, "direction": "forward" }
            ]
        },
        "lights": {
            "head_light": { "hardware": ["L1"], "brightness_max": 100 },
            "tail_light": { "hardware": ["L2"], "brightness_max": 70 }
        }
    }"#;

    let config = parse_hardware_config(sample_config_json).expect("valid config");
    let mut pin_pool = DynamicPinPool::new();

    let bound = BoundHardware::bind::<MikroV2>(&config, &mut pin_pool)
        .expect("Hardware pin binding succeeded");

    println!("Bound Headlight GPIOs: {:?}", bound.head_light_gpios.as_slice());
    println!("Bound Taillight GPIOs: {:?}", bound.tail_light_gpios.as_slice());

    let vehicle = VehicleController::new(VehicleType::Truck, rc_core::DrivetrainMode::Ackermann);
    println!("Mikro V2 vehicle controller initialized: state={:?}", vehicle.engine.state);
}
