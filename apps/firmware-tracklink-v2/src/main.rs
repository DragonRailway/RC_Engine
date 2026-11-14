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

    // Physical Hardware Actuators on TrackLink V2:
    // DRIVER_A: PWM1 (GPIO 6), PWM2 (GPIO 7), Enable (GPIO 5)
    // Lights: L1 (GPIO 9 / Headlight), L2 (GPIO 10 / Reversing/Tail), L0 (GPIO 42 / Status)
    let mut motor_pwm1 = Output::new(peripherals.GPIO6, Level::Low, OutputConfig::default());
    let mut motor_pwm2 = Output::new(peripherals.GPIO7, Level::Low, OutputConfig::default());
    let mut motor_en   = Output::new(peripherals.GPIO5, Level::High, OutputConfig::default());
    let mut led_l1     = Output::new(peripherals.GPIO9, Level::Low, OutputConfig::default());
    let mut led_l2     = Output::new(peripherals.GPIO10, Level::Low, OutputConfig::default());
    let mut led_l0     = Output::new(peripherals.GPIO42, Level::Low, OutputConfig::default());

    motor_pwm1.set_low();
    motor_pwm2.set_low();
    motor_en.set_high(); // Enable H-bridge
    led_l1.set_low();
    led_l2.set_low();
    led_l0.set_low();

    // Setup Locomotive RadioKit UI layout (Portrait mode):
    // Wid 10: Throttle (Slider 0..100)
    // Wid 11: Reverser (Button toggle: FWD / REV)
    // Wid 12: Headlight (Button toggle: Headlight / Reversing Light)
    // Wid 13: Bell (Button: Sound/Bell trigger)
    // Wid 14: Engine (Button toggle: Engine Run / Stop)
    let mut storage = [WidgetDescriptor::default(); RK_MAX_WIDGETS];
    let mut runner = RadioKitBuilder::new("TrackLink-V2")
        .theme("rk")
        .orientation(Orientation::Portrait)
        .add_slider(10, "Throttle", 75, 105, 26, 104, CenteringMode::Min)
        .add_button(11, "Reverser", 75, 170, 20, None)
        .add_button(12, "Headlight", 28, 105, 20, None)
        .add_button(13, "Bell", 28, 140, 20, Some("bell"))
        .add_button(14, "Engine", 28, 170, 20, Some("power"))
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

    let mut engine_running = false;
    let mut prev_engine_btn = false;
    let mut reverser_fwd = true;
    let mut prev_reverser_btn = false;
    let mut headlight_on = false;
    let mut prev_headlight_btn = false;

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
                                let mut offset = 0;
                                while offset < pkt_len {
                                    let chunk_size = (pkt_len - offset).min(64);
                                    let _ = server.service.widget.notify_raw(&gatt_conn, &pkt_buf[offset..offset + chunk_size], false).await;
                                    offset += chunk_size;
                                }
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
                                            let mut offset = 0;
                                            while offset < pkt_len {
                                                let chunk_size = (pkt_len - offset).min(64);
                                                let _ = server.service.widget.notify_raw(&gatt_conn, &pkt_buf[offset..offset + chunk_size], false).await;
                                                offset += chunk_size;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        _ => {}
                    }

                    // ── Locomotive Vehicle Controller Logic ──
                    // 1. Engine Start / Stop Toggle (Wid 14)
                    let engine_btn = runner.get_value(14) == Some(&WidgetValue::Button(true));
                    if engine_btn && !prev_engine_btn {
                        engine_running = !engine_running;
                    }
                    prev_engine_btn = engine_btn;

                    // 2. Reverser Toggle (Wid 11: FWD / REV)
                    let rev_btn = runner.get_value(11) == Some(&WidgetValue::Button(true));
                    if rev_btn && !prev_reverser_btn {
                        reverser_fwd = !reverser_fwd;
                    }
                    prev_reverser_btn = rev_btn;

                    // 3. Headlight Toggle (Wid 12)
                    let head_btn = runner.get_value(12) == Some(&WidgetValue::Button(true));
                    if head_btn && !prev_headlight_btn {
                        headlight_on = !headlight_on;
                    }
                    prev_headlight_btn = head_btn;

                    // 4. Bell Active (Wid 13)
                    let bell_active = runner.get_value(13) == Some(&WidgetValue::Button(true));

                    // 5. Throttle Slider (Wid 10: 0..100)
                    let raw_throttle = match runner.get_value(10) {
                        Some(&WidgetValue::Slider(val)) => val.clamp(0, 100) as u8,
                        _ => 0,
                    };

                    // Only produce motion when engine is running
                    let effective_throttle = if engine_running { raw_throttle } else { 0 };

                    // Actuate Driver A (GPIO 6 & 7) based on reverser direction and throttle
                    if effective_throttle > 5 {
                        if reverser_fwd {
                            motor_pwm1.set_high();
                            motor_pwm2.set_low();
                        } else {
                            motor_pwm1.set_low();
                            motor_pwm2.set_high();
                        }
                    } else {
                        motor_pwm1.set_low();
                        motor_pwm2.set_low();
                    }

                    // Directional Lighting:
                    // When headlight_on is active:
                    // - FWD: L1 (Headlight) ON, L2 (Tail/Reversing) OFF
                    // - REV: L2 (Tail/Reversing) ON, L1 (Headlight) OFF
                    if headlight_on {
                        if reverser_fwd {
                            led_l1.set_high();
                            led_l2.set_low();
                        } else {
                            led_l1.set_low();
                            led_l2.set_high();
                        }
                    } else {
                        led_l1.set_low();
                        led_l2.set_low();
                    }

                    // Status LED (GPIO 42 / L0):
                    // Active when engine is running or bell is sounding
                    if engine_running || bell_active {
                        led_l0.set_high();
                    } else {
                        led_l0.set_low();
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
