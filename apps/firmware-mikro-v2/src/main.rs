//! Dedicated firmware application for Mikro V2.

use bsp_mikro_v2::MikroV2;
use rc_brain::*;
use rk_rs::{WidgetDescriptor, RK_MAX_WIDGETS};

fn main() {
    let hardware = MikroV2::init_hardware();
    println!("=== Booting RC Brain on {} ===", hardware.name);

    let mut storage = [WidgetDescriptor::default(); RK_MAX_WIDGETS];
    let mut runtime = VehicleRuntime::new::<MikroV2>(hardware, &mut storage);

    let power_off = runtime.step::<MikroV2>(false, 20);
    println!("Mikro V2 runtime stepped: power_off={}", power_off);
}
