//! Real-time multi-channel audio synthesizer and DSP engine for RC Engine.

#![no_std]

#[cfg(feature = "std")]
extern crate std;

pub mod hermite;
pub mod knock;
pub mod limiter;
pub mod mixer;
pub mod sample;

pub use hermite::read_interpolated_hermite_4p;
pub use knock::{KnockPattern, KnockTracker};
pub use limiter::saturate_soft_knee;
pub use mixer::{Mixer, VoiceState};
pub use sample::{SoundBank, SoundId, SoundSlot, TOTAL_SOUND_COUNT};
