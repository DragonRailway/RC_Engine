//! End-to-end integration test for RC Brain:
//! RadioKit UI commands -> 50 Hz Vehicle Control -> Actuators -> Audio State -> Telemetry.

use rc_brain::*;
use rc_core::{
    battery::BatteryState,
    engine_sim::EngineState,
    lights::HeadlightMode,
};
use rk_rs::{
    RadioKitConfig, RadioKitRunner, WidgetDescriptor, WidgetValue, RK_MAX_WIDGETS,
};

#[test]
fn test_vehicle_full_control_cycle() {
    let mut storage = [WidgetDescriptor::default(); RK_MAX_WIDGETS];
    let count = build_cockpit_descriptors(&mut storage);
    let mut rk = RadioKitRunner::new(
        RadioKitConfig {
            name: "Test Vehicle",
            theme: "dragon",
            orientation: rk_rs::Orientation::Landscape,
        },
        &storage[..count],
    );

    let audio_shared = AudioSharedState::default();
    let mut vehicle = VehicleController::default();
    vehicle.battery.auto_detect_cells(8.0); // 2S battery ~8.0V

    // Initially: Engine is Off, motors are 0
    assert_eq!(vehicle.engine.state, EngineState::Off);
    assert_eq!(vehicle.motor_outputs.drive, 0);

    // 1. User presses ignition START button
    rk.set_value(WID_START_BUTTON, WidgetValue::Button(true));
    vehicle.read_inputs(&rk);
    vehicle.step(20, &audio_shared);

    assert_eq!(vehicle.engine.state, EngineState::Starting);
    assert_eq!(audio_shared.engine_state.load(core::sync::atomic::Ordering::Relaxed), 1);

    // Release button
    rk.set_value(WID_START_BUTTON, WidgetValue::Button(false));
    vehicle.read_inputs(&rk);

    // Step through starter duration (1200 ms = 60 ticks of 20 ms)
    for _ in 0..60 {
        vehicle.step(20, &audio_shared);
    }

    assert_eq!(vehicle.engine.state, EngineState::Running);
    assert_eq!(audio_shared.engine_state.load(core::sync::atomic::Ordering::Relaxed), 2);
    assert_eq!(vehicle.engine.current_rpm as u16, 600); // Idle RPM

    // 2. User presses gas pedal to 80%
    rk.set_value(WID_GAS_PEDAL, WidgetValue::Slider(80));
    vehicle.read_inputs(&rk);

    // Step 20 ticks (400 ms) to let flywheel inertia ramp RPM
    for _ in 0..20 {
        vehicle.step(20, &audio_shared);
    }

    assert!(vehicle.engine.current_rpm > 1000.0);
    assert_eq!(vehicle.motor_outputs.drive, 80);
    assert!(audio_shared.rpm.load(core::sync::atomic::Ordering::Relaxed) > 1000);

    // 3. User turns steering knob to 50 (right turn)
    rk.set_value(WID_STEERING, WidgetValue::Knob(50));
    vehicle.read_inputs(&rk);
    vehicle.step(20, &audio_shared);

    assert_eq!(vehicle.motor_outputs.steering, 50);

    // 4. User turns on headlights (HighBeam = 2)
    rk.set_value(WID_TRUCK_LIGHT, WidgetValue::Multiple(2));
    vehicle.read_inputs(&rk);
    vehicle.step(20, &audio_shared);

    assert_eq!(vehicle.lights.headlight_mode, HeadlightMode::HighBeam);
    assert_eq!(vehicle.light_outputs.headlights, 100);
    assert_eq!(vehicle.light_outputs.high_beams, 100);

    // 5. User applies brake pedal (Jake brake engages and drive motor stops)
    rk.set_value(WID_BRAKE_PEDAL, WidgetValue::Slider(70));
    vehicle.read_inputs(&rk);
    vehicle.step(20, &audio_shared);

    assert!(vehicle.jake_brake_active);
    assert_eq!(vehicle.motor_outputs.drive, 0);
    assert_eq!(vehicle.light_outputs.brake_lights, 100);
    assert!(audio_shared.jake_brake_active.load(core::sync::atomic::Ordering::Relaxed));

    // 6. User presses horn
    rk.set_value(WID_HORN_BUTTON, WidgetValue::Button(true));
    vehicle.read_inputs(&rk);
    vehicle.step(20, &audio_shared);

    assert!(vehicle.horn_active);
    assert!(audio_shared.horn_active.load(core::sync::atomic::Ordering::Relaxed));

    // 7. Telemetry update check
    for _ in 0..15 {
        vehicle.step(20, &audio_shared);
        vehicle.update_telemetry(&mut rk);
    }

    if let Some(WidgetValue::Text(batt)) = rk.get_value(WID_TELEM_BATTERY) {
        assert!(batt.contains('V'), "Battery telemetry should contain 'V': {}", batt);
    } else {
        panic!("Battery telemetry not updated");
    }

    if let Some(WidgetValue::Text(speed)) = rk.get_value(WID_TELEM_SPEED) {
        assert!(speed.contains("km/h"), "Speed telemetry should contain 'km/h': {}", speed);
    } else {
        panic!("Speed telemetry not updated");
    }

    // 8. Low-voltage cutoff safety check
    // Simulate battery drops below cutoff (3.0V per cell * 2 = 6.0V)
    for _ in 0..60 {
        vehicle.battery.update(5.5, 20);
        vehicle.step(20, &audio_shared);
    }

    assert_eq!(vehicle.battery.state, BatteryState::Cutoff);
    // Throttle demand of 80% should be completely inhibited
    rk.set_value(WID_BRAKE_PEDAL, WidgetValue::Slider(0));
    rk.set_value(WID_GAS_PEDAL, WidgetValue::Slider(80));
    vehicle.read_inputs(&rk);
    vehicle.step(20, &audio_shared);

    assert_eq!(vehicle.motor_outputs.drive, 0, "Motor drive must be 0 during cutoff");
}

#[test]
fn test_wav_to_rp_pcm_conversion() {
    use rc_brain::assets::converter::wav_to_rp_pcm;
    use rc_dsp::SoundSlot;

    // Create a minimal 16-bit mono 22050 Hz WAV header with 4 samples
    let sample_rate: u32 = 22050;
    let num_samples: usize = 4;
    let data_len = (num_samples * 2) as u32;
    let riff_len = 36 + data_len;

    let mut wav: Vec<u8> = Vec::new();
    wav.extend_from_slice(b"RIFF");
    wav.extend_from_slice(&riff_len.to_le_bytes());
    wav.extend_from_slice(b"WAVE");
    wav.extend_from_slice(b"fmt ");
    wav.extend_from_slice(&16u32.to_le_bytes());
    wav.extend_from_slice(&1u16.to_le_bytes()); // PCM
    wav.extend_from_slice(&1u16.to_le_bytes()); // Mono
    wav.extend_from_slice(&sample_rate.to_le_bytes());
    wav.extend_from_slice(&(sample_rate * 2).to_le_bytes());
    wav.extend_from_slice(&2u16.to_le_bytes());
    wav.extend_from_slice(&16u16.to_le_bytes()); // 16-bit
    wav.extend_from_slice(b"data");
    wav.extend_from_slice(&data_len.to_le_bytes());

    // 4 samples: 0, 16384 (positive), -16384 (negative), 32767 (max)
    let samples: [i16; 4] = [0, 16384, -16384, 32767];
    for s in samples {
        wav.extend_from_slice(&s.to_le_bytes());
    }

    let pcm = wav_to_rp_pcm(&wav).expect("WAV conversion should succeed");
    assert_eq!(&pcm[0..2], b"RP");
    assert_eq!(u16::from_le_bytes([pcm[2], pcm[3]]), 22050);
    assert_eq!(u32::from_le_bytes([pcm[4], pcm[5], pcm[6], pcm[7]]), 4);

    // Verify it parses cleanly with SoundSlot
    let slot = SoundSlot::parse_pcm(&pcm).expect("RP PCM must be parseable");
    assert_eq!(slot.len(), 4);
    assert_eq!(slot.samples[0], 0);
    assert_eq!(slot.samples[1], 64);
    assert_eq!(slot.samples[2], -64);
    assert_eq!(slot.samples[3], 127);
}
