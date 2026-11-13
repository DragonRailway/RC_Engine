//! RC Engine embedded vehicle firmware library.
//!
//! Integrates 50 Hz vehicle dynamics, 22.05 kHz dual-core audio synthesis,
//! lighting automation, battery safety monitoring, and the RadioKit BLE actor.

#![no_std]

#[cfg(feature = "std")]
extern crate std;

pub mod assets;
pub mod audio_pipeline;
pub mod cockpit;
pub mod runtime;
pub mod vehicle_task;

pub use assets::*;
pub use audio_pipeline::*;
pub use cockpit::*;
pub use runtime::*;
pub use vehicle_task::*;
