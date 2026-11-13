//! Real-time audio pipeline and lockless dual-core state synchronization.

use core::sync::atomic::{AtomicBool, AtomicU16, AtomicU8, Ordering};
use rc_dsp::{KnockPattern, KnockTracker, Mixer, SoundBank, SoundId};

/// Lockless state shared between the 50 Hz control task (Core 0) and the audio task (Core 1).
pub struct AudioSharedState {
    pub rpm: AtomicU16,
    pub engine_state: AtomicU8, // 0 = Off, 1 = Starting, 2 = Running, 3 = Stopping
    pub horn_active: AtomicBool,
    pub jake_brake_active: AtomicBool,
    pub master_volume: AtomicU8,
}

impl Default for AudioSharedState {
    fn default() -> Self {
        Self {
            rpm: AtomicU16::new(0),
            engine_state: AtomicU8::new(0),
            horn_active: AtomicBool::new(false),
            jake_brake_active: AtomicBool::new(false),
            master_volume: AtomicU8::new(100),
        }
    }
}

/// Dedicated audio synthesizer worker driving I2S DMA buffers.
pub struct AudioWorker<'a> {
    pub mixer: Mixer<'a>,
    pub knock_tracker: KnockTracker,
    pub idle_rpm: u16,
    prev_state: u8,
}

impl<'a> AudioWorker<'a> {
    pub fn new(bank: SoundBank<'a>, pattern: KnockPattern, idle_rpm: u16) -> Self {
        Self {
            mixer: Mixer::new(bank),
            knock_tracker: KnockTracker::new(pattern),
            idle_rpm,
            prev_state: 0,
        }
    }

    /// Process a frame block: synchronizes atomic parameters and renders 16-bit PCM samples.
    pub fn process_block(&mut self, shared: &AudioSharedState, block: &mut [i16]) {
        let rpm = shared.rpm.load(Ordering::Relaxed);
        let state = shared.engine_state.load(Ordering::Relaxed);
        let horn = shared.horn_active.load(Ordering::Relaxed);
        let jake = shared.jake_brake_active.load(Ordering::Relaxed);
        let master = shared.master_volume.load(Ordering::Relaxed);

        self.mixer.master_volume = master;

        // State transition detection
        if state != self.prev_state {
            match state {
                1 => {
                    // Starting
                    self.mixer.play(SoundId::Start, 100, 1.0, false);
                }
                2 => {
                    // Running
                    self.mixer.play(SoundId::Idle, 80, 1.0, true);
                    self.mixer.play(SoundId::Rev, 70, 1.0, true);
                    self.mixer.play(SoundId::Turbo, 40, 1.0, true);
                    self.knock_tracker.reset();
                }
                0 | 3 => {
                    // Stopping or Off
                    self.mixer.stop(SoundId::Idle);
                    self.mixer.stop(SoundId::Rev);
                    self.mixer.stop(SoundId::Turbo);
                    self.mixer.stop(SoundId::Knock);
                }
                _ => {}
            }
            self.prev_state = state;
        }

        // Running engine pitch modulation
        if state == 2 {
            let base_rpm = (self.idle_rpm.max(1)) as f32;
            let pitch_ratio = (rpm as f32) / base_rpm;

            // Idle fades out as rev fades in
            let idle_step = pitch_ratio.clamp(0.8, 1.6);
            let rev_step = (pitch_ratio * 0.9).clamp(0.9, 3.2);
            let turbo_step = (0.7 + pitch_ratio * 0.6).clamp(0.7, 3.5);

            self.mixer.set_pitch(SoundId::Idle, idle_step);
            self.mixer.set_pitch(SoundId::Rev, rev_step);
            self.mixer.set_pitch(SoundId::Turbo, turbo_step);

            // Cross-fading idle and rev based on RPM
            if rpm > self.idle_rpm + 200 {
                self.mixer.set_volume(SoundId::Idle, 20);
                self.mixer.set_volume(SoundId::Rev, 100);
            } else {
                self.mixer.set_volume(SoundId::Idle, 80);
                self.mixer.set_volume(SoundId::Rev, 30);
            }
        }

        // Horn handling
        if horn {
            if !self.mixer.voices[SoundId::Horn.index()].active {
                self.mixer.play(SoundId::Horn, 100, 1.0, true);
            }
        } else {
            self.mixer.stop(SoundId::Horn);
        }

        // Jake brake handling
        if jake && state == 2 {
            if !self.mixer.voices[SoundId::JakeBrake.index()].active {
                self.mixer.play(SoundId::JakeBrake, 100, 1.0, true);
            }
        } else {
            self.mixer.stop(SoundId::JakeBrake);
        }

        // Render audio into caller-provided DMA block
        self.mixer.render_block(block);
    }
}
