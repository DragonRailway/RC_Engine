//! Host test verifying WAV rendering of an RPM sweep with Hermite interpolation.

use rc_dsp::*;

/// Helper to write a 16-bit mono 22,050 Hz WAV file
fn write_wav_mono(path: &str, samples: &[i16], sample_rate: u32) -> std::io::Result<()> {
    use std::fs::File;
    use std::io::Write;

    let mut file = File::create(path)?;
    let data_len = (samples.len() * 2) as u32;
    let file_len = 36 + data_len;

    // RIFF header
    file.write_all(b"RIFF")?;
    file.write_all(&file_len.to_le_bytes())?;
    file.write_all(b"WAVE")?;

    // fmt subchunk
    file.write_all(b"fmt ")?;
    file.write_all(&16u32.to_le_bytes())?; // Subchunk1Size (16 for PCM)
    file.write_all(&1u16.to_le_bytes())?;  // AudioFormat (1 for PCM)
    file.write_all(&1u16.to_le_bytes())?;  // NumChannels (1 mono)
    file.write_all(&sample_rate.to_le_bytes())?;
    let byte_rate = sample_rate * 2;
    file.write_all(&byte_rate.to_le_bytes())?;
    file.write_all(&2u16.to_le_bytes())?;  // BlockAlign
    file.write_all(&16u16.to_le_bytes())?; // BitsPerSample

    // data subchunk
    file.write_all(b"data")?;
    file.write_all(&data_len.to_le_bytes())?;

    for &sample in samples {
        file.write_all(&sample.to_le_bytes())?;
    }

    Ok(())
}

#[test]
fn test_render_rpm_sweep_wav() {
    // Generate a single-cycle engine idle pulse (64 samples of 8-bit PCM)
    let mut idle_cycle = [0i8; 64];
    for (i, sample) in idle_cycle.iter_mut().enumerate() {
        let phase = (i as f32) / 64.0 * core::f32::consts::TAU;
        // Asymmetric diesel pulse
        let val = (phase.sin() * 80.0 + (phase * 2.0).sin() * 40.0) as i8;
        *sample = val;
    }

    let mut bank = SoundBank::new();
    bank.set_slot(SoundId::Idle, SoundSlot::new(&idle_cycle, 22050));

    let mut mixer = Mixer::new(bank);
    mixer.play(SoundId::Idle, 100, 1.0, true);

    const SAMPLE_RATE: usize = 22050;
    const DURATION_SECS: usize = 2;
    const TOTAL_SAMPLES: usize = SAMPLE_RATE * DURATION_SECS;

    let mut output = vec![0i16; TOTAL_SAMPLES];
    let mut block = [0i16; 64];

    for chunk_idx in 0..(TOTAL_SAMPLES / 64) {
        // Sweep pitch step from 0.8 (idle) up to 2.5 (rev)
        let progress = (chunk_idx * 64) as f32 / TOTAL_SAMPLES as f32;
        let pitch_step = 0.8 + 1.7 * (progress * core::f32::consts::PI).sin();
        mixer.set_pitch(SoundId::Idle, pitch_step);

        mixer.render_block(&mut block);
        let start = chunk_idx * 64;
        output[start..start + 64].copy_from_slice(&block);
    }

    // Verify valid audio output
    assert!(output.iter().any(|&s| s != 0));

    // Write output WAV file for developer inspection
    let wav_path = "/home/sun/Apps/RCKIT/rc-brain-rs/test_engine_sweep.wav";
    let _ = write_wav_mono(wav_path, &output, 22050);
}
