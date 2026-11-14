//! Production firmware binary for TrackLink V2 - LED & Hardware Control Test.

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
use rk_rs::*;

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
    log::info!("TrackLink V2 booting... initializing LED test hardware");

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

    // Physical Hardware LED Outputs on TrackLink V2:
    // GPIO 9 (L1) and GPIO 42 (L0 / Built-in LED)
    let mut led_l1 = Output::new(peripherals.GPIO9, Level::Low, OutputConfig::default());
    let mut led_l0 = Output::new(peripherals.GPIO42, Level::Low, OutputConfig::default());
    led_l1.set_low();
    led_l0.set_low();

    // Setup RadioKit UI with Button (Toggle), Flash Button, and Feedback LED
    let mut storage = [WidgetDescriptor::default(); RK_MAX_WIDGETS];
    let mut runner = RadioKitBuilder::new("TrackLink-V2")
        .theme("rk")
        .orientation(Orientation::Landscape)
        .add_button(1, "Lights", 20, 20, 25, None)
        .add_button(2, "Flash", 80, 20, 25, None)
        .add_led(3, "L0", 140, 20, 20, LedShape::Circle)
        .build(&mut storage);

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

    let ble_runner = stack.runner();
    let mut peripheral = stack.peripheral();

    let server = RadioKitServer::new_with_config(GapConfig::Peripheral(PeripheralConfig {
        name: "TrackLink-V2",
        appearance: &appearance::UNKNOWN,
    }))
    .unwrap();

    let token = ble_runner_task(ble_runner).unwrap();
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

    let mut l1_state = false;

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
            if let Ok(gatt_conn) = conn.with_attribute_server(&server) {
                let mut conf_pushed = false;
                loop {
                    // Check if companion subscribed to widget CCCD and push CONF_DATA
                    if !conf_pushed && server.service.widget.should_notify(&gatt_conn) {
                        let mut p_buf = [0u8; 128];
                        if let Ok(p_len) = build_conf_payload(
                            &mut p_buf,
                            runner.config.orientation,
                            runner.config.theme,
                            0,
                            1,
                            &[],
                            0,
                            runner.widgets,
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
                                let mut is_get_conf = false;
                                write_evt.with_data(|_offset, data| {
                                    let mut parser = WidgetFrameParser::new();
                                    for &b in data {
                                        if let Some(Ok(cmd)) = parser.feed_byte(b) {
                                            if cmd == Command::GetConf {
                                                is_get_conf = true;
                                            } else if cmd == Command::VarUpdate || cmd == Command::SetInput {
                                                runner.handle_packet(cmd, parser.payload());
                                            }
                                        }
                                    }
                                });
                                let _ = write_evt.accept().map(|resp| resp.send());

                                if is_get_conf {
                                    let mut p_buf = [0u8; 128];
                                    if let Ok(p_len) = build_conf_payload(
                                        &mut p_buf,
                                        runner.config.orientation,
                                        runner.config.theme,
                                        0,
                                        1,
                                        &[],
                                        0,
                                        runner.widgets,
                                    ) {
                                        let mut pkt_buf = [0u8; 192];
                                        if let Ok(pkt_len) = build_packet(&mut pkt_buf, Command::ConfData, &p_buf[..p_len]) {
                                            let mut notif_val = [0u8; 64];
                                            let copy_len = pkt_len.min(64);
                                            notif_val[..copy_len].copy_from_slice(&pkt_buf[..copy_len]);
                                            let _ = server.service.widget.notify(&gatt_conn, &notif_val, false).await;
                                        }
                                    }
                                }
                            }
                        }
                        _ => {}
                    }

                    // Check widget states and drive physical LEDs
                    let btn1 = runner.get_value(1) == Some(&WidgetValue::Button(true));
                    let btn2 = runner.get_value(2) == Some(&WidgetValue::Button(true));

                    if btn1 {
                        l1_state = !l1_state;
                    }

                    // L1 (GPIO 9) controlled by Headlights toggle
                    if l1_state {
                        led_l1.set_high();
                    } else {
                        led_l1.set_low();
                    }

                    // L0 (GPIO 42) active when Flash button is pressed or L1 is active
                    if btn2 || l1_state {
                        led_l0.set_high();
                        runner.set_value(3, WidgetValue::Led { state: true, color: 0x00FF00, opacity: 255 });
                    } else {
                        led_l0.set_low();
                        runner.set_value(3, WidgetValue::Led { state: false, color: 0x00FF00, opacity: 50 });
                    }

                    // Power latch check
                    if hw_power.is_button_pressed() {
                        // Keep alive
                    }
                }
            }
        }
    }
}
