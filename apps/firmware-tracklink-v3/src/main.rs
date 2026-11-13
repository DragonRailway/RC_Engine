//! Dedicated firmware application for TrackLink V3.

use bsp_tracklink_v3::TrackLinkV3;
use rc_brain::*;
use rk_rs::{WidgetDescriptor, RK_MAX_WIDGETS};

fn main() {
    let hardware = TrackLinkV3::init_hardware();
    println!("=== Booting RC Brain on {} ===", hardware.name);

    let mut storage = [WidgetDescriptor::default(); RK_MAX_WIDGETS];
    let mut runtime = VehicleRuntime::new::<TrackLinkV3>(hardware, &mut storage);

    // Simulate standard vehicle run step
    let power_off = runtime.step::<TrackLinkV3>(false, 20);
    println!("TrackLink V3 runtime stepped: power_off={}", power_off);
}
