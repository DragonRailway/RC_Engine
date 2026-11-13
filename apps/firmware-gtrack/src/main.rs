//! Dedicated firmware application for GTrack Dual-Track Excavators.

use bsp_core::{Board, DynamicPinPool};
use bsp_gtrack::GTrack;
use rc_brain::*;
use rc_core::{hardware_config::*, VehicleType};

fn main() {
    println!("=== Booting RC Brain on {} ===", GTrack::NAME);
    println!("Power Latch Pin: GPIO {:?}", GTrack::POWER.power_enable);
    println!("5V Servo Buck Enable: GPIO {:?}", GTrack::POWER.servo_enable);
    println!("Battery Voltage Divider Ratio (20k/5.1k): {:.4}", GTrack::VOLTAGE_DIVIDER.ratio());

    let sample_config_json = br#"{
        "drivetrain": {
            "left_motor": { "hardware": "DRIVER_A", "frequency": 20000, "direction": "forward" },
            "right_motor": { "hardware": "DRIVER_B", "frequency": 20000, "direction": "forward" }
        },
        "lights": {
            "head_light": { "hardware": ["L1"], "brightness_max": 100 }
        }
    }"#;

    let config = parse_hardware_config(sample_config_json).expect("valid config");
    let mut pin_pool = DynamicPinPool::new();

    let bound = BoundHardware::bind::<GTrack>(&config, &mut pin_pool)
        .expect("Hardware pin binding succeeded");

    println!("Bound Headlight GPIOs: {:?}", bound.head_light_gpios.as_slice());

    let mut vehicle = VehicleController::new(VehicleType::Excavator, rc_core::DrivetrainMode::SkidSteer);
    vehicle.battery.warning_voltage_per_cell = 3.4;

    println!(
        "GTrack firmware initialized with Skid-Steer dual track mode: engine={:?}",
        vehicle.engine.state
    );
}
