//! Multi-channel 32-voice audio synthesizer mixer.

use crate::hermite::read_interpolated_hermite_4p;
use crate::limiter::saturate_soft_knee;
use crate::sample::{SoundBank, SoundId, TOTAL_SOUND_COUNT};

#[cfg(not(feature = "std"))]
use libm::fmodf;

#[cfg(feature = "std")]
#[inline]
fn fmodf(x: f32, y: f32) -> f32 {
    x % y
}

/// Runtime playback state for a single synthesizer voice.
#[derive(Debug, Clone, Copy)]
pub struct VoiceState {
    pub position: f32,
    pub step: f32,
    pub volume: u8,
    pub active: bool,
    pub loop_mode: bool,
    pub loop_begin: usize,
    pub loop_end: usize,
}

impl Default for VoiceState {
    fn default() -> Self {
        Self {
            position: 0.0,
            step: 1.0,
            volume: 100,
            active: false,
            loop_mode: true,
            loop_begin: 0,
            loop_end: 0,
        }
    }
}

impl VoiceState {
    /// Advance the voice playhead by `step`.
    ///
    /// Returns `true` if the playhead wrapped or reached the end of the sample.
    #[inline]
    pub fn advance(&mut self, sample_count: usize) -> bool {
        if sample_count == 0 {
            self.active = false;
            return false;
        }

        self.position += self.step;
        let count_f = sample_count as f32;

        if self.position >= count_f {
            if self.loop_mode && self.loop_end > self.loop_begin && self.loop_end <= sample_count {
                let region_len = (self.loop_end - self.loop_begin) as f32;
                if region_len > 0.0 {
                    let offset = fmodf(self.position - (self.loop_begin as f32), region_len);
                    self.position = (self.loop_begin as f32) + offset;
                    if self.position >= (self.loop_end as f32) {
                        self.position = self.loop_begin as f32;
                    }
                } else {
                    self.position = self.loop_begin as f32;
                }
            } else if self.loop_mode {
                self.position = fmodf(self.position, count_f);
            } else {
                self.active = false;
                self.position = count_f;
            }
            return true;
        }

        false
    }
}

/// Real-time 32-voice audio mixer.
pub struct Mixer<'a> {
    pub bank: SoundBank<'a>,
    pub voices: [VoiceState; TOTAL_SOUND_COUNT],
    pub master_volume: u8,
}

impl<'a> Mixer<'a> {
    pub const fn new(bank: SoundBank<'a>) -> Self {
        Self {
            bank,
            voices: [VoiceState {
                position: 0.0,
                step: 1.0,
                volume: 100,
                active: false,
                loop_mode: true,
                loop_begin: 0,
                loop_end: 0,
            }; TOTAL_SOUND_COUNT],
            master_volume: 100,
        }
    }

    /// Trigger a sound slot with given volume, pitch step, and looping option.
    pub fn play(&mut self, id: SoundId, volume: u8, step: f32, loop_mode: bool) {
        let v = &mut self.voices[id.index()];
        v.position = 0.0;
        v.volume = volume;
        v.step = step;
        v.loop_mode = loop_mode;
        v.active = true;
    }

    /// Stop playback of a voice immediately.
    pub fn stop(&mut self, id: SoundId) {
        self.voices[id.index()].active = false;
    }

    /// Update pitch playback step ratio (1.0 = native pitch).
    pub fn set_pitch(&mut self, id: SoundId, step: f32) {
        self.voices[id.index()].step = step;
    }

    /// Update volume (0 to 100).
    pub fn set_volume(&mut self, id: SoundId, volume: u8) {
        self.voices[id.index()].volume = volume;
    }

    /// Set loop boundaries in sample frames.
    pub fn set_loop_points(&mut self, id: SoundId, begin: usize, end: usize) {
        let v = &mut self.voices[id.index()];
        v.loop_begin = begin;
        v.loop_end = end;
    }

    /// Render a block of 16-bit signed PCM audio into `out_buffer`.
    ///
    /// Applies 4-point Hermite interpolation per active voice, mixes with volume
    /// weighting, and saturates through the soft-knee limiter.
    pub fn render_block(&mut self, out_buffer: &mut [i16]) {
        let master_scale = (self.master_volume as f32) / 100.0;

        for out_sample in out_buffer.iter_mut() {
            let mut mix_accum: f32 = 0.0;

            for i in 0..TOTAL_SOUND_COUNT {
                let voice = &mut self.voices[i];
                if !voice.active {
                    continue;
                }

                let slot = &self.bank.slots[i];
                if slot.is_empty() {
                    voice.active = false;
                    continue;
                }

                let raw = read_interpolated_hermite_4p(
                    slot.samples,
                    voice.position,
                    voice.loop_mode,
                    voice.loop_begin,
                    voice.loop_end,
                );

                // 8-bit sample (-128..127) scaled to 16-bit range (* 256.0)
                let gain = ((voice.volume as f32) / 100.0) * master_scale * 256.0;
                mix_accum += raw * gain;

                voice.advance(slot.len());
            }

            *out_sample = saturate_soft_knee(mix_accum);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::sample::SoundSlot;

    #[test]
    fn test_mixer_silence_when_inactive() {
        let bank = SoundBank::new();
        let mut mixer = Mixer::new(bank);
        let mut buffer = [0i16; 64];

        mixer.render_block(&mut buffer);
        for &sample in &buffer {
            assert_eq!(sample, 0);
        }
    }

    #[test]
    fn test_mixer_play_and_advance() {
        // Simple sawtooth pattern 8-bit PCM
        static SAW: [i8; 4] = [0, 40, 80, 120];

        let mut bank = SoundBank::new();
        bank.set_slot(SoundId::Idle, SoundSlot::new(&SAW, 22050));

        let mut mixer = Mixer::new(bank);
        mixer.play(SoundId::Idle, 100, 1.0, true);

        let mut buffer = [0i16; 4];
        mixer.render_block(&mut buffer);

        // First sample should be around 0
        assert_eq!(buffer[0], 0);
        // Second sample around 40 * 256 = 10240
        assert!((buffer[1] - 10240).abs() < 100);

        // After 4 steps with step=1.0, position should loop
        assert!(mixer.voices[SoundId::Idle.index()].active);
    }
}
