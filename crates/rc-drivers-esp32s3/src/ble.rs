//! BLE GATT peripheral server for ESP32-S3 using esp-radio, esp-rtos, and trouble-host.

use bt_hci::controller::ExternalController;
use esp_radio::ble::controller::BleConnector;
use static_cell::StaticCell;
use trouble_host::prelude::*;

pub type BleController = ExternalController<BleConnector<'static>, 20>;

#[gatt_server]
pub struct RadioKitServer {
    pub service: RadioKitService,
}

#[gatt_service(uuid = "0000FFE0-0000-1000-8000-00805F9B34FB")]
pub struct RadioKitService {
    #[characteristic(uuid = "0000FFE1-0000-1000-8000-00805F9B34FB", write, write_without_response, notify, value = [0u8; 64])]
    pub widget: [u8; 64],

    #[characteristic(uuid = "0000FFE2-0000-1000-8000-00805F9B34FB", write, write_without_response, notify, value = [0u8; 64])]
    pub fs: [u8; 64],

    #[characteristic(uuid = "0000FFE3-0000-1000-8000-00805F9B34FB", write, write_without_response, notify, value = [0u8; 64])]
    pub ota: [u8; 64],

    #[characteristic(uuid = "0000FFE4-0000-1000-8000-00805F9B34FB", write, write_without_response, notify, value = [0u8; 64])]
    pub settings: [u8; 64],

    #[characteristic(uuid = "0000FFE5-0000-1000-8000-00805F9B34FB", notify, value = [0u8; 64])]
    pub print: [u8; 64],
}

#[embassy_executor::task]
pub async fn ble_runner_task(mut runner: Runner<'static, BleController, DefaultPacketPool>) {
    loop {
        let _ = runner.run().await;
        embassy_time::Timer::after(embassy_time::Duration::from_millis(50)).await;
    }
}
