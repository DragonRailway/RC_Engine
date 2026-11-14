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
extern crate alloc;
#[cfg(feature = "embedded")]
extern crate esp_alloc;

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
    timer::timg::TimerGroup,
    main,
};

#[cfg(feature = "embedded")]
use bt_hci::controller::ExternalController;
#[cfg(feature = "embedded")]
use esp_radio::ble::controller::BleConnector;
#[cfg(feature = "embedded")]
use rc_drivers_esp32s3::{ble_runner_task, BleController, HardwarePower, RadioKitServer};
#[cfg(feature = "embedded")]
use static_cell::StaticCell;
#[cfg(feature = "embedded")]
use trouble_host::prelude::*;

use bsp_core::Board;
use bsp_tracklink_v2::TrackLinkV2;
use rc_brain::*;
use rk_rs::{WidgetDescriptor, RK_MAX_WIDGETS};
use rk_rs::{build_conf_payload, build_packet, Command, Orientation};

#[cfg(feature = "embedded")]
#[main]
async fn main(spawner: embassy_executor::Spawner) {
    let config = esp_hal::Config::default().with_cpu_clock(CpuClock::max());
    let peripherals = esp_hal::init(config);

    // Initialize 72 KB heap for esp-radio and BLE stack
    esp_alloc::heap_allocator!(size: 72 * 1024);

    // Initialize esp-rtos scheduler and system timer
    let timg0 = TimerGroup::new(peripherals.TIMG0);
    esp_rtos::start(timg0.timer0, peripherals.FROM_CPU_INTR0);

    // Initialize esp-println logger over USB-Serial-JTAG
    esp_println::logger::init_logger(log::LevelFilter::Info);
    log::info!("TrackLink V2 booting... initializing hardware");

    // Initialize TrackLink V2 Power Latch (GPIO21) and Button (GPIO38)
    let mut pwr_latch = Output::new(peripherals.GPIO21, Level::High, OutputConfig::default());
    pwr_latch.set_high(); // Latch power immediately!
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

    // Initialize BLE Stack with esp-radio + trouble-host
    let transport = BleConnector::new(peripherals.BT, esp_radio::ble::Config::default()).unwrap();
    let controller: BleController = ExternalController::new(transport);

    static RESOURCES: StaticCell<HostResources<BleController, DefaultPacketPool, 1, 1>> = StaticCell::new();
    let resources = RESOURCES.init(HostResources::new());

    static STACK: StaticCell<Stack<'static, BleController, DefaultPacketPool>> = StaticCell::new();
    let stack = STACK.init(
        trouble_host::new(controller, resources)
            .set_random_address(Address::random([0xff, 0x8f, 0x1a, 0x05, 0xe4, 0xff]))
            .build(),
    );

    let runner = stack.runner();
    let mut peripheral = stack.peripheral();

    let server = RadioKitServer::new_with_config(GapConfig::Peripheral(PeripheralConfig {
        name: "TrackLink-V2",
        appearance: &appearance::UNKNOWN,
    }))
    .unwrap();

    let token = ble_runner_task(runner).unwrap();
    spawner.spawn(token);

    // BLE Advertising packets
    let mut adv_data = [0u8; 31];
    let adv_data_len = AdStructure::encode_slice(
        &[
            AdStructure::Flags(LE_GENERAL_DISCOVERABLE | BR_EDR_NOT_SUPPORTED),
            AdStructure::CompleteServiceUuids16(&[[0xE0, 0xFF]]),
        ],
        &mut adv_data[..],
    )
    .unwrap();

    let mut scan_data = [0u8; 31];
    let scan_data_len = AdStructure::encode_slice(
        &[AdStructure::CompleteLocalName(b"TrackLink-V2")],
        &mut scan_data[..],
    )
    .unwrap();

    loop {
        let acceptor = peripheral
            .advertise(
                &Default::default(),
                Advertisement::ConnectableScannableUndirected {
                    adv_data: &adv_data[..adv_data_len],
                    scan_data: &scan_data[..scan_data_len],
                },
            )
            .await
            .unwrap();

        if let Ok(conn) = acceptor.accept().await {
            if let Ok(mut gatt_conn) = conn.with_attribute_server(&server) {
                let mut conf_pushed = false;
                loop {
                    if !conf_pushed && server.service.widget.should_notify(&gatt_conn) {
                        let mut p_buf = [0u8; 128];
                        if let Ok(p_len) = build_conf_payload(
                            &mut p_buf,
                            Orientation::Landscape,
                            "dragon",
                            0,
                            1,
                            &[],
                            0,
                            runtime.rk_runner.widgets,
                        ) {
                            let mut pkt_buf = [0u8; 192];
                            if let Ok(pkt_len) = build_packet(&mut pkt_buf, Command::ConfData, &p_buf[..p_len]) {
                                let mut notif_val = [0u8; 64];
                                let copy_len = pkt_len.min(64);
                                notif_val[..copy_len].copy_from_slice(&pkt_buf[..copy_len]);
                                let _ = server.service.widget.notify(&gatt_conn, &notif_val, false).await;
                                conf_pushed = true;
                            }
                        }
                    }

                    match gatt_conn.next().await {
                        GattConnectionEvent::Disconnected { .. } => break,
                        GattConnectionEvent::Gatt { event } => {
                            if let GattEvent::Write(write_evt) = event {
                                let _ = write_evt.accept().map(|resp| resp.send());
                            }
                        }
                        _ => {}
                    }

                    let btn_pressed = hw_power.is_button_pressed();
                    let shutdown = runtime.step::<TrackLinkV2>(btn_pressed, 20);
                    if shutdown {
                        hw_power.assert_latch(false);
                        loop {}
                    }
                }
            }
        }
    }
}

#[cfg(not(feature = "embedded"))]
fn main() {
    println!("Host simulation mode for TrackLink V2");
}
