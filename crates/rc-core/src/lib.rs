//! Vehicle physics, motion controller, lighting sequencer, and battery safety.

#![no_std]

pub mod battery;
pub mod engine_sim;
pub mod hardware_config;
pub mod lights;
pub mod motion;

pub use battery::{BatteryMonitor, BatteryState};
pub use engine_sim::{EngineConfig, EngineSim, EngineState, VehicleType};
pub use hardware_config::*;
pub use lights::{HeadlightMode, LightOutputs, LightingSequencer, TurnMode};
pub use motion::{DrivetrainMode, MotionController, MotorOutputs};
