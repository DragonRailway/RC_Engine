//! Production firmware binary for TrackLink V3.
//!
//! Compiles both for host simulation (`feature = "std"`) and bare-metal ESP32-S3 (`feature = "embedded"`).

#![cfg_attr(not(feature = "std"), no_std)]
#![cfg_attr(not(feature = "std"), no_main)]

#[cfg(feature = "embedded")]
#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    esp_hal::system::software_reset()
}

#[cfg(feature = "embedded")]
esp_bootloader_esp_idf::esp_app_desc!();

#[cfg(feature = "embedded")]
use esp_hal::{
    clock::CpuClock,
    gpio::{Input, InputConfig, Level, Output, OutputConfig, Pull},
    main,
};

#[cfg(feature = "embedded")]
use rc_drivers_esp32s3::HardwarePower;

use bsp_core::Board;
use bsp_tracklink_v3::TrackLinkV3;
use rc_brain::*;
use rk_rs::{WidgetDescriptor, RK_MAX_WIDGETS};

#[cfg(feature = "embedded")]
#[main]
fn main() -> ! {
    let config = esp_hal::Config::default().with_cpu_clock(CpuClock::max());
    let peripherals = esp_hal::init(config);

    // Initialize Power Latch and Button
    let pwr_latch = Output::new(peripherals.GPIO35, Level::High, OutputConfig::default());
    let pwr_btn = Input::new(peripherals.GPIO36, InputConfig::default().with_pull(Pull::Up));

    let mut hw_power = HardwarePower::new(
        Some(pwr_latch),
        Some(pwr_btn),
        TrackLinkV3::POWER.power_enable,
        TrackLinkV3::POWER.power_button,
    );

    let hardware = TrackLinkV3::init_hardware();
    let mut storage = [WidgetDescriptor::default(); RK_MAX_WIDGETS];
    let mut runtime = VehicleRuntime::new::<TrackLinkV3>(hardware, &mut storage);

    loop {
        // Step vehicle runtime at 50 Hz (20 ms dt)
        let btn_pressed = hw_power.is_button_pressed();
        let shutdown = runtime.step::<TrackLinkV3>(btn_pressed, 20);
        if shutdown {
            hw_power.assert_latch(false);
            loop {}
        }

        esp_hal::delay::Delay::new().delay_millis(20);
    }
}

#[cfg(feature = "std")]
fn main() {
    let hardware = TrackLinkV3::init_hardware();
    println!("=== Booting RC Brain Simulation on {} ===", hardware.name);

    let mut storage = [WidgetDescriptor::default(); RK_MAX_WIDGETS];
    let mut runtime = VehicleRuntime::new::<TrackLinkV3>(hardware, &mut storage);

    // Run one test step in simulation
    let power_off = runtime.step::<TrackLinkV3>(false, 20);
    println!("TrackLink V3 runtime stepped: power_off={}", power_off);
}
