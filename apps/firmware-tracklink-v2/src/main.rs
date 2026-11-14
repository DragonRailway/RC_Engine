//! Production firmware binary for TrackLink V2.
//!
//! Pin assignments:
//! - Power latch: GPIO21
//! - Power button: GPIO38 (active low)
//! - Motor Driver A: GPIO6, GPIO7 (PWM), GPIO5 (Enable)
//! - I2S DAC: LRC GPIO15, BCLK GPIO16, DIN GPIO17, SD GPIO18

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
use bsp_tracklink_v2::TrackLinkV2;
use rc_brain::*;
use rk_rs::{WidgetDescriptor, RK_MAX_WIDGETS};

#[cfg(feature = "embedded")]
#[main]
fn main() -> ! {
    let config = esp_hal::Config::default().with_cpu_clock(CpuClock::max());
    let peripherals = esp_hal::init(config);

    // Initialize TrackLink V2 Power Latch (GPIO21) and Button (GPIO38)
    let pwr_latch = Output::new(peripherals.GPIO21, Level::High, OutputConfig::default());
    let pwr_btn = Input::new(peripherals.GPIO38, InputConfig::default().with_pull(Pull::Up));

    let mut hw_power = HardwarePower::new(
        Some(pwr_latch),
        Some(pwr_btn),
        TrackLinkV2::POWER.power_enable,
        TrackLinkV2::POWER.power_button,
    );

    let hardware = TrackLinkV2::init_hardware();
    let mut storage = [WidgetDescriptor::default(); RK_MAX_WIDGETS];
    let mut runtime = VehicleRuntime::new::<TrackLinkV2>(hardware, &mut storage);

    let delay = esp_hal::delay::Delay::new();

    loop {
        // Step vehicle runtime at 50 Hz (20 ms dt)
        let btn_pressed = hw_power.is_button_pressed();
        let shutdown = runtime.step::<TrackLinkV2>(btn_pressed, 20);
        if shutdown {
            hw_power.assert_latch(false);
            loop {}
        }

        delay.delay_millis(20);
    }
}

#[cfg(feature = "std")]
fn main() {
    let hardware = TrackLinkV2::init_hardware();
    println!("=== Booting RC Brain Simulation on {} ===", hardware.name);

    let mut storage = [WidgetDescriptor::default(); RK_MAX_WIDGETS];
    let mut runtime = VehicleRuntime::new::<TrackLinkV2>(hardware, &mut storage);

    let power_off = runtime.step::<TrackLinkV2>(false, 20);
    println!("TrackLink V2 runtime stepped: power_off={}", power_off);
}
