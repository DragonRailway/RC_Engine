//! Embedded vehicle sound assets and WAV to RP-PCM conversion tooling.

use rc_dsp::{SoundBank, SoundId, SoundSlot};

pub static RAW_SCANIA_IDLE: &[u8] = include_bytes!("../sounds/scania_v8/idle.pcm");
pub static RAW_SCANIA_REV: &[u8] = include_bytes!("../sounds/scania_v8/rev.pcm");
pub static RAW_SCANIA_START: &[u8] = include_bytes!("../sounds/scania_v8/start.pcm");
pub static RAW_SCANIA_HORN: &[u8] = include_bytes!("../sounds/scania_v8/horn.pcm");
pub static RAW_SCANIA_JAKE: &[u8] = include_bytes!("../sounds/scania_v8/jakebrake.pcm");
pub static RAW_SCANIA_TURBO: &[u8] = include_bytes!("../sounds/scania_v8/turbo.pcm");

/// Load the embedded Scania V8 sound bank zero-copy directly from flash/ROM.
pub fn load_scania_v8_soundbank() -> SoundBank<'static> {
    let mut bank = SoundBank::new();

    if let Ok(slot) = SoundSlot::parse_pcm(RAW_SCANIA_IDLE) {
        bank.set_slot(SoundId::Idle, slot);
    }
    if let Ok(slot) = SoundSlot::parse_pcm(RAW_SCANIA_REV) {
        bank.set_slot(SoundId::Rev, slot);
    }
    if let Ok(slot) = SoundSlot::parse_pcm(RAW_SCANIA_START) {
        bank.set_slot(SoundId::Start, slot);
    }
    if let Ok(slot) = SoundSlot::parse_pcm(RAW_SCANIA_HORN) {
        bank.set_slot(SoundId::Horn, slot);
    }
    if let Ok(slot) = SoundSlot::parse_pcm(RAW_SCANIA_JAKE) {
        bank.set_slot(SoundId::JakeBrake, slot);
    }
    if let Ok(slot) = SoundSlot::parse_pcm(RAW_SCANIA_TURBO) {
        bank.set_slot(SoundId::Turbo, slot);
    }

    bank
}

#[cfg(feature = "std")]
pub mod converter {
    use std::vec::Vec;

    #[derive(Debug, PartialEq, Eq)]
    pub enum WavConvertError {
        InvalidRiff,
        UnsupportedFormat,
        MissingDataChunk,
    }

    /// Converts a standard mono WAV file (8-bit or 16-bit PCM) into the 8-byte
    /// RP header `.pcm` format required by RC Brain.
    pub fn wav_to_rp_pcm(wav: &[u8]) -> Result<Vec<u8>, WavConvertError> {
        if wav.len() < 44 || &wav[0..4] != b"RIFF" || &wav[8..12] != b"WAVE" {
            return Err(WavConvertError::InvalidRiff);
        }

        let mut offset = 12;
        let mut sample_rate = 22050u16;
        let mut bits_per_sample = 16u16;
        let mut pcm_samples: Vec<i8> = Vec::new();

        while offset + 8 <= wav.len() {
            let chunk_id = &wav[offset..offset + 4];
            let chunk_len = u32::from_le_bytes([
                wav[offset + 4],
                wav[offset + 5],
                wav[offset + 6],
                wav[offset + 7],
            ]) as usize;
            offset += 8;

            if chunk_id == b"fmt " && chunk_len >= 16 {
                let audio_fmt = u16::from_le_bytes([wav[offset], wav[offset + 1]]);
                let channels = u16::from_le_bytes([wav[offset + 2], wav[offset + 3]]);
                sample_rate = u32::from_le_bytes([
                    wav[offset + 4],
                    wav[offset + 5],
                    wav[offset + 6],
                    wav[offset + 7],
                ]) as u16;
                bits_per_sample = u16::from_le_bytes([wav[offset + 14], wav[offset + 15]]);

                if audio_fmt != 1 || channels != 1 {
                    return Err(WavConvertError::UnsupportedFormat);
                }
            } else if chunk_id == b"data" {
                let data_slice = &wav[offset..offset.saturating_add(chunk_len).min(wav.len())];
                if bits_per_sample == 8 {
                    // Standard WAV 8-bit is unsigned (0..255), convert to signed i8 (-128..127)
                    for &b in data_slice {
                        pcm_samples.push((b as i16 - 128) as i8);
                    }
                } else if bits_per_sample == 16 {
                    // Standard WAV 16-bit is signed i16 little-endian, downsample to i8
                    for chunk in data_slice.chunks_exact(2) {
                        let val = i16::from_le_bytes([chunk[0], chunk[1]]);
                        pcm_samples.push((val >> 8) as i8);
                    }
                }
                break;
            }

            offset += chunk_len;
        }

        if pcm_samples.is_empty() {
            return Err(WavConvertError::MissingDataChunk);
        }

        // Build 8-byte header: "RP" + u16 rate + u32 count + raw i8 samples
        let mut out = Vec::with_capacity(8 + pcm_samples.len());
        out.extend_from_slice(b"RP");
        out.extend_from_slice(&sample_rate.to_le_bytes());
        out.extend_from_slice(&(pcm_samples.len() as u32).to_le_bytes());
        for s in pcm_samples {
            out.push(s as u8);
        }

        Ok(out)
    }
}
