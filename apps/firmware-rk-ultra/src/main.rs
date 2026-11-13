//! Dedicated firmware application for RK_ULTRA.

use bsp_rk_ultra::RkUltra;
use rc_brain::*;
use rk_rs::{WidgetDescriptor, RK_MAX_WIDGETS};

fn main() {
    let hardware = RkUltra::init_hardware();
    println!("=== Booting RC Brain on {} ===", hardware.name);

    let mut storage = [WidgetDescriptor::default(); RK_MAX_WIDGETS];
    let mut runtime = VehicleRuntime::new::<RkUltra>(hardware, &mut storage);

    let power_off = runtime.step::<RkUltra>(false, 20);
    println!("RK_ULTRA runtime stepped: power_off={}", power_off);
}
