//! Dedicated firmware application for GTrack Dual-Track Excavators.

use bsp_gtrack::GTrack;
use rc_brain::*;
use rk_rs::{WidgetDescriptor, RK_MAX_WIDGETS};

fn main() {
    let hardware = GTrack::init_hardware();
    println!("=== Booting RC Brain on {} ===", hardware.name);

    let mut storage = [WidgetDescriptor::default(); RK_MAX_WIDGETS];
    let mut runtime = VehicleRuntime::new::<GTrack>(hardware, &mut storage);

    let power_off = runtime.step::<GTrack>(false, 20);
    println!("GTrack runtime stepped: power_off={}", power_off);
}
