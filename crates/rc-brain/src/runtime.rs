//! Central engine runtime for ESP32-S3 (Xtensa and RISC-V targets).
//!
//! Handles:
//! - Setting up the 2-page RadioKit cockpit
//! - Initializing the audio worker and lockless shared audio state
//! - Spawning Core 1 I2S DMA audio streaming (or cooperative task on single-core)
//! - Executing the 50 Hz Embassy vehicle physics, lighting, and power button control loop

use bsp_core::Board;
use rc_drivers_esp32s3::BoardHardware;
use rc_dsp::KnockPattern;
use rk_rs::{
    RadioKitConfig, RadioKitRunner, WidgetDescriptor, RK_MAX_WIDGETS,
};

use crate::assets::load_scania_v8_soundbank;
use crate::audio_pipeline::{AudioSharedState, AudioWorker};
use crate::cockpit::build_cockpit_descriptors;
use crate::vehicle_task::VehicleController;

/// The main execution loop of the vehicle.
///
/// Can run on physical bare-metal hardware or in simulation.
pub fn run_vehicle_step<B: Board>(
    vehicle: &mut VehicleController,
    hardware: &mut BoardHardware,
    audio_shared: &AudioSharedState,
    audio_worker: &mut AudioWorker,
    rk_runner: &mut RadioKitRunner,
    audio_buf: &mut [i16],
    button_pressed: bool,
    dt_ms: u16,
) -> bool {
    // 1. Evaluate power button & latch state machine
    let should_power_off = hardware.power.update(button_pressed, dt_ms);
    if should_power_off {
        return true; // Signal shutdown
    }

    // 2. Read RadioKit UI inputs
    vehicle.read_inputs(rk_runner);

    // 3. Step physics simulation, motion mixing, lighting, and battery monitoring
    vehicle.step(dt_ms, audio_shared);

    // 4. Update live telemetry back to RadioKit companion app
    vehicle.update_telemetry(rk_runner);

    // 5. Render audio frame block (on dual-core ESP32-S3, this runs on Core 1)
    audio_worker.process_block(audio_shared, audio_buf);

    false
}

/// Standalone runner helper for a board target.
pub struct VehicleRuntime<'a> {
    pub vehicle: VehicleController,
    pub hardware: BoardHardware,
    pub audio_shared: AudioSharedState,
    pub audio_worker: AudioWorker<'a>,
    pub rk_runner: RadioKitRunner<'a>,
    pub audio_block: [i16; 128],
}

impl<'a> VehicleRuntime<'a> {
    pub fn new<B: Board>(
        hardware: BoardHardware,
        widget_storage: &'a mut [WidgetDescriptor<'a>; RK_MAX_WIDGETS],
    ) -> Self {
        let count = build_cockpit_descriptors(widget_storage);
        let rk_runner = RadioKitRunner::new(
            RadioKitConfig {
                name: B::NAME,
                theme: "dragon",
                orientation: rk_rs::Orientation::Landscape,
            },
            &widget_storage[..count],
        );

        let sound_bank = load_scania_v8_soundbank();
        let audio_shared = AudioSharedState::default();
        let audio_worker = AudioWorker::new(sound_bank, KnockPattern::V8, 600);
        let vehicle = VehicleController::default();

        Self {
            vehicle,
            hardware,
            audio_shared,
            audio_worker,
            rk_runner,
            audio_block: [0i16; 128],
        }
    }

    /// Step the runtime by `dt_ms` (typically 20 ms @ 50 Hz).
    pub fn step<B: Board>(&mut self, button_pressed: bool, dt_ms: u16) -> bool {
        run_vehicle_step::<B>(
            &mut self.vehicle,
            &mut self.hardware,
            &self.audio_shared,
            &mut self.audio_worker,
            &mut self.rk_runner,
            &mut self.audio_block,
            button_pressed,
            dt_ms,
        )
    }
}
