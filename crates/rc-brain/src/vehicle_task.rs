//! 50 Hz vehicle control loop linking RadioKit inputs, physics simulation,
//! motion actuators, lighting sequences, battery telemetry, and audio state.

use core::sync::atomic::Ordering;
use heapless::String;
use rc_core::{
    battery::{BatteryMonitor, BatteryState},
    engine_sim::{EngineConfig, EngineSim, EngineState, VehicleType},
    lights::{HeadlightMode, LightOutputs, LightingSequencer, TurnMode},
    motion::{DrivetrainMode, MotionController, MotorOutputs},
};
use rk_rs::{RadioKitRunner, WidgetValue};

use crate::audio_pipeline::AudioSharedState;
use crate::cockpit::*;

/// Primary vehicle controller operating at 50 Hz (20 ms period).
pub struct VehicleController {
    pub vehicle_type: VehicleType,
    pub engine: EngineSim,
    pub motion: MotionController,
    pub lights: LightingSequencer,
    pub battery: BatteryMonitor,

    // Runtime state from cockpit
    pub user_throttle: i16,
    pub user_steering: i16,
    pub user_brake: u8,
    pub horn_active: bool,
    pub jake_brake_active: bool,
    pub gear: i8, // 1 = Fwd, 0 = Neutral, -1 = Rev

    // Actuator outputs
    pub motor_outputs: MotorOutputs,
    pub light_outputs: LightOutputs,

    // Telemetry scheduler
    telemetry_timer_ms: u16,
    prev_start_btn: bool,
}

impl Default for VehicleController {
    fn default() -> Self {
        Self::new(VehicleType::Truck, DrivetrainMode::Ackermann)
    }
}

impl VehicleController {
    pub fn new(vehicle_type: VehicleType, drivetrain: DrivetrainMode) -> Self {
        Self {
            vehicle_type,
            engine: EngineSim::new(EngineConfig::default()),
            motion: MotionController::new(drivetrain),
            lights: LightingSequencer::new(),
            battery: BatteryMonitor::default(),

            user_throttle: 0,
            user_steering: 0,
            user_brake: 0,
            horn_active: false,
            jake_brake_active: false,
            gear: 1, // Default forward

            motor_outputs: MotorOutputs::default(),
            light_outputs: LightOutputs::default(),

            telemetry_timer_ms: 0,
            prev_start_btn: false,
        }
    }

    /// Read incoming RadioKit widget values and map them to control variables.
    pub fn read_inputs(&mut self, runner: &RadioKitRunner) {
        // Page 0: Truck Controls
        if let Some(WidgetValue::Knob(val)) = runner.get_value(WID_STEERING) {
            // Knob is -128..127 or 0..100 -> map to -100..100
            self.user_steering = (*val as i16).clamp(-100, 100);
        }

        let mut gas = 0i16;
        if let Some(WidgetValue::Slider(val)) = runner.get_value(WID_GAS_PEDAL) {
            gas = (*val as i16).clamp(0, 100);
        }

        let mut brake = 0u8;
        if let Some(WidgetValue::Slider(val)) = runner.get_value(WID_BRAKE_PEDAL) {
            brake = (*val as u8).clamp(0, 100);
        }
        self.user_brake = brake;

        // Gear switch (0: Fwd, 1: Neutral, 2: Rev)
        if let Some(WidgetValue::Multiple(val)) = runner.get_value(WID_GEAR_SWITCH) {
            self.gear = match val {
                0 => 1,
                1 => 0,
                2 => -1,
                _ => 1,
            };
        }

        // Net throttle based on gear and pedals
        if brake > 5 {
            self.user_throttle = 0;
            self.jake_brake_active = true;
        } else {
            self.jake_brake_active = false;
            self.user_throttle = match self.gear {
                1 => gas,
                -1 => -gas,
                _ => 0,
            };
        }

        // Start / Stop Engine button
        if let Some(WidgetValue::Button(pressed)) = runner.get_value(WID_START_BUTTON) {
            if *pressed && !self.prev_start_btn {
                // Rising edge toggle
                match self.engine.state {
                    EngineState::Off => self.engine.start(),
                    EngineState::Running => self.engine.stop(),
                    _ => {}
                }
            }
            self.prev_start_btn = *pressed;
        }

        // Horn Button
        if let Some(WidgetValue::Button(pressed)) = runner.get_value(WID_HORN_BUTTON) {
            self.horn_active = *pressed;
        }

        // Truck Headlight Selection (Multiple)
        if let Some(WidgetValue::Multiple(mode)) = runner.get_value(WID_TRUCK_LIGHT) {
            self.lights.headlight_mode = match mode {
                0 => HeadlightMode::Off,
                1 => HeadlightMode::LowBeam,
                2 => HeadlightMode::HighBeam,
                _ => HeadlightMode::Off,
            };
        }

        // Indicators
        let left_ind = matches!(runner.get_value(WID_LEFT_INDICATOR), Some(WidgetValue::Button(true)));
        let right_ind = matches!(runner.get_value(WID_RIGHT_INDICATOR), Some(WidgetValue::Button(true)));
        if left_ind && right_ind {
            self.lights.turn_mode = TurnMode::Hazard;
        } else if left_ind {
            self.lights.turn_mode = TurnMode::Left;
        } else if right_ind {
            self.lights.turn_mode = TurnMode::Right;
        } else {
            self.lights.turn_mode = TurnMode::Off;
        }

        // Page 1: Loco Controls override if in Loco mode
        if self.vehicle_type == VehicleType::Locomotive {
            if let Some(WidgetValue::Slider(val)) = runner.get_value(WID_LOCO_THROTTLE) {
                let fwd = matches!(runner.get_value(WID_DIR_SWITCH), Some(WidgetValue::SlideSwitch(true)));
                let raw = (*val as i16).clamp(0, 100);
                self.user_throttle = if fwd { raw } else { -raw };
            }
            if let Some(WidgetValue::Button(pressed)) = runner.get_value(WID_BELL_BUTTON) {
                self.horn_active = *pressed;
            }
        }
    }

    /// Step the 50 Hz control loop (dt = 20 ms).
    ///
    /// Updates physics, lighting, motion mixing, battery monitoring, and syncs
    /// with the lockless audio state.
    pub fn step(&mut self, dt_ms: u16, audio_state: &AudioSharedState) {
        // 1. Safety check: Battery low-voltage cutoff
        let effective_throttle = if self.battery.state == BatteryState::Cutoff {
            0
        } else if self.engine.state != EngineState::Running {
            0
        } else {
            self.user_throttle
        };

        // 2. Engine simulation step
        let throttle_abs = (effective_throttle.abs() as u8).min(100);
        self.engine.update(throttle_abs, self.jake_brake_active, dt_ms);

        // 3. Motion control mixing
        self.motor_outputs = self.motion.update(effective_throttle, self.user_steering, 0);

        // 4. Lighting automation step
        self.light_outputs = self.lights.update(effective_throttle, self.user_steering, dt_ms);

        // 5. Update lockless audio shared state
        let state_u8 = match self.engine.state {
            EngineState::Off => 0,
            EngineState::Starting => 1,
            EngineState::Running => 2,
            EngineState::Stopping => 3,
            EngineState::ParkingBrake => 0,
        };
        audio_state.engine_state.store(state_u8, Ordering::Relaxed);
        audio_state.rpm.store(self.engine.current_rpm as u16, Ordering::Relaxed);
        audio_state.horn_active.store(self.horn_active, Ordering::Relaxed);
        audio_state.jake_brake_active.store(self.jake_brake_active, Ordering::Relaxed);

        self.telemetry_timer_ms += dt_ms;
    }

    /// Update telemetry variables in RadioKit if the telemetry interval (e.g. 200 ms) has elapsed.
    pub fn update_telemetry(&mut self, runner: &mut RadioKitRunner) {
        if self.telemetry_timer_ms >= 200 {
            self.telemetry_timer_ms = 0;

            // Push Battery Telemetry string (e.g. "7.4V")
            let mut batt_str: String<32> = String::new();
            let v = self.battery.filtered_voltage;
            let v_int = v as u32;
            let v_frac = ((v - (v_int as f32)) * 10.0) as u32;
            let _ = core::fmt::write(&mut batt_str, format_args!("{}.{}V", v_int, v_frac));
            runner.set_value(WID_TELEM_BATTERY, WidgetValue::Text(batt_str));

            // Push Speed Telemetry string (e.g. "15 km/h")
            let mut speed_str: String<32> = String::new();
            let est_speed = ((self.motor_outputs.drive.abs() as f32) * 0.25) as u32;
            let _ = core::fmt::write(&mut speed_str, format_args!("{} km/h", est_speed));
            runner.set_value(WID_TELEM_SPEED, WidgetValue::Text(speed_str));
        }
    }
}
