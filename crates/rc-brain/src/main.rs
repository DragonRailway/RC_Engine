//! Host simulator and runner for RC Brain.
//!
//! Demonstrates vehicle control loop, RadioKit UI updates, and real-time audio
//! synthesis rendering into an output WAV file.

#[cfg(feature = "std")]
fn main() {
    use std::fs::File;
    use std::io::Write;
    use rc_brain::*;
    use rc_dsp::KnockPattern;
    use rk_rs::{
        RadioKitConfig, RadioKitRunner, WidgetDescriptor, WidgetValue, RK_MAX_WIDGETS,
    };

    println!("=== RC Brain Rust Simulation & Audio Synthesis ===");

    // 1. Build RadioKit Cockpit
    let mut storage = [WidgetDescriptor::default(); RK_MAX_WIDGETS];
    let count = build_cockpit_descriptors(&mut storage);
    let mut rk = RadioKitRunner::new(
        RadioKitConfig {
            name: "RC Scania V8",
            theme: "dragon",
            orientation: rk_rs::Orientation::Landscape,
        },
        &storage[..count],
    );

    // 2. Load SoundBank & Init Audio
    let bank = load_scania_v8_soundbank();
    let shared_audio = AudioSharedState::default();
    let mut audio_worker = AudioWorker::new(bank, KnockPattern::V8, 600);

    // 3. Init Vehicle Controller
    let mut vehicle = VehicleController::default();
    vehicle.battery.auto_detect_cells(7.4); // 2S LiPo

    // 4. Simulate a driving session over 5 seconds (250 ticks @ 50 Hz = 20 ms per tick)
    let sample_rate = 22050usize;
    let samples_per_tick = sample_rate / 50; // 441 samples per 20 ms tick
    let mut total_rendered_samples: Vec<i16> = Vec::with_capacity(sample_rate * 5);
    let mut block = vec![0i16; samples_per_tick];

    println!("Simulating 5 seconds of driving:");
    println!("  - 0.0s: Ignition Start");
    println!("  - 1.5s: Engine Idle reached, Throttle ramp up");
    println!("  - 3.0s: Horn honk & Jake braking");
    println!("  - 4.0s: Returning to idle");

    for tick in 0..250 {
        let time_ms = tick * 20;

        // Simulate incoming RadioKit companion app commands at milestones
        if tick == 5 {
            // Press start button
            rk.set_value(WID_START_BUTTON, WidgetValue::Button(true));
        } else if tick == 10 {
            // Release start button
            rk.set_value(WID_START_BUTTON, WidgetValue::Button(false));
        } else if tick == 75 {
            // Accelerate throttle to 75%
            rk.set_value(WID_GAS_PEDAL, WidgetValue::Slider(75));
        } else if tick == 150 {
            // Press Horn and engage brake
            rk.set_value(WID_HORN_BUTTON, WidgetValue::Button(true));
            rk.set_value(WID_BRAKE_PEDAL, WidgetValue::Slider(80));
            rk.set_value(WID_GAS_PEDAL, WidgetValue::Slider(0));
        } else if tick == 180 {
            // Release horn and release brake
            rk.set_value(WID_HORN_BUTTON, WidgetValue::Button(false));
            rk.set_value(WID_BRAKE_PEDAL, WidgetValue::Slider(0));
        }

        // 50 Hz Vehicle Task Step
        vehicle.read_inputs(&rk);
        vehicle.step(20, &shared_audio);
        vehicle.update_telemetry(&mut rk);

        // Core 1 Audio Task: Render 20 ms audio block
        audio_worker.process_block(&shared_audio, &mut block);
        total_rendered_samples.extend_from_slice(&block);

        if tick % 50 == 0 {
            println!(
                "  [{:4} ms] State={:?} RPM={:4} Drive={:4} Steer={:4}",
                time_ms,
                vehicle.engine.state,
                vehicle.engine.current_rpm as u16,
                vehicle.motor_outputs.drive,
                vehicle.motor_outputs.steering
            );
        }
    }

    // 5. Output rendered audio to WAV file
    let wav_path = "scania_v8_simulation.wav";
    let data_bytes = (total_rendered_samples.len() * 2) as u32;
    let riff_size = 36 + data_bytes;
    let byte_rate = (sample_rate as u32) * 2;

    let mut wav_file = File::create(wav_path).expect("Failed to create wav file");
    let mut header = [0u8; 44];
    header[0..4].copy_from_slice(b"RIFF");
    header[4..8].copy_from_slice(&riff_size.to_le_bytes());
    header[8..12].copy_from_slice(b"WAVE");
    header[12..16].copy_from_slice(b"fmt ");
    header[16..20].copy_from_slice(&16u32.to_le_bytes());
    header[20..22].copy_from_slice(&1u16.to_le_bytes()); // PCM
    header[22..24].copy_from_slice(&1u16.to_le_bytes()); // Mono
    header[24..28].copy_from_slice(&(sample_rate as u32).to_le_bytes());
    header[28..32].copy_from_slice(&byte_rate.to_le_bytes());
    header[32..34].copy_from_slice(&2u16.to_le_bytes()); // block align
    header[34..36].copy_from_slice(&16u16.to_le_bytes()); // 16 bits
    header[36..40].copy_from_slice(b"data");
    header[40..44].copy_from_slice(&data_bytes.to_le_bytes());

    wav_file.write_all(&header).unwrap();
    for sample in total_rendered_samples {
        wav_file.write_all(&sample.to_le_bytes()).unwrap();
    }

    println!("Simulation complete! Rendered WAV saved to {}", wav_path);
}

#[cfg(not(feature = "std"))]
fn main() {}
