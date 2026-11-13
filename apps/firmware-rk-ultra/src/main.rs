//! Dedicated firmware application for RK_ULTRA.

use bsp_core::{Board, DynamicPinPool};
use bsp_rk_ultra::{ImuPins, RkUltra};
use rc_brain::*;
use rc_core::{hardware_config::*, VehicleType};

fn main() {
    println!("=== Booting RC Brain on {} ===", RkUltra::NAME);
    println!("Power Latch Pin: GPIO {:?}", RkUltra::POWER.power_enable);
    println!("Battery Voltage Divider Ratio (75k/10k): {:.2}", RkUltra::VOLTAGE_DIVIDER.ratio());
    println!("IMU SPI CS Pin: GPIO {}", ImuPins::CS);
    println!("High-side Accessory Pump Pin: GPIO {:?}", RkUltra::POWER.pump_enable);

    let sample_config_json = br#"{
        "drivetrain": {
            "drive_motors": [
                { "hardware": "DRIVER_A", "frequency": 25000, "direction": "forward" },
                { "hardware": "DRIVER_B", "frequency": 25000, "direction": "forward" }
            ]
        },
        "lights": {
            "head_light": { "hardware": ["A1"], "brightness_max": 100 }
        }
    }"#;

    let config = parse_hardware_config(sample_config_json).expect("valid config");
    let mut pin_pool = DynamicPinPool::new();

    let bound = BoundHardware::bind::<RkUltra>(&config, &mut pin_pool)
        .expect("Hardware pin binding succeeded");

    println!("Bound Headlight GPIOs: {:?}", bound.head_light_gpios.as_slice());

    let mut vehicle = VehicleController::new(VehicleType::Truck, rc_core::DrivetrainMode::Ackermann);
    vehicle.battery.warning_voltage_per_cell = 3.5;

    println!(
        "RK_ULTRA firmware initialized successfully: engine={:?}, warn={:.1}V",
        vehicle.engine.state,
        vehicle.battery.warning_voltage_per_cell
    );
}
