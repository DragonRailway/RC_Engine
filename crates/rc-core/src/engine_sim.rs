//! Vehicle engine physics and real-time RPM simulation.

/// Operating state of the simulated combustion engine.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum EngineState {
    #[default]
    Off,
    Starting,
    Running,
    Stopping,
    ParkingBrake,
}

/// Simulated vehicle type determining default control mechanics.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum VehicleType {
    #[default]
    Truck,
    Locomotive,
    Excavator,
}

/// Dynamic engine physics configuration.
#[derive(Debug, Clone)]
pub struct EngineConfig {
    pub min_rpm: u16,
    pub max_rpm: u16,
    pub idle_rpm: u16,
    pub inertia: u8,
    pub max_pitch_factor: f32,
    pub start_duration_ms: u16,
    pub stop_duration_ms: u16,
    pub jake_brake_decel_rate: u8,
}

impl Default for EngineConfig {
    fn default() -> Self {
        Self {
            min_rpm: 300,
            max_rpm: 2500,
            idle_rpm: 600,
            inertia: 8,
            max_pitch_factor: 3.5,
            start_duration_ms: 1200,
            stop_duration_ms: 1000,
            jake_brake_decel_rate: 15,
        }
    }
}

/// Real-time engine simulator tracking RPM, flywheel inertia, and state transitions.
pub struct EngineSim {
    pub config: EngineConfig,
    pub state: EngineState,
    pub current_rpm: f32,
    pub target_rpm: f32,
    pub pitch_ratio: f32,
    state_timer_ms: u16,
}

impl Default for EngineSim {
    fn default() -> Self {
        Self::new(EngineConfig::default())
    }
}

impl EngineSim {
    pub const fn new(config: EngineConfig) -> Self {
        Self {
            config,
            state: EngineState::Off,
            current_rpm: 0.0,
            target_rpm: 0.0,
            pitch_ratio: 1.0,
            state_timer_ms: 0,
        }
    }

    /// Request the engine to start cranking.
    pub fn start(&mut self) {
        if self.state == EngineState::Off {
            self.state = EngineState::Starting;
            self.state_timer_ms = 0;
            self.current_rpm = 150.0; // Cranking starter RPM
        }
    }

    /// Request the engine to shut down.
    pub fn stop(&mut self) {
        if self.state == EngineState::Running {
            self.state = EngineState::Stopping;
            self.state_timer_ms = 0;
        }
    }

    /// Step the engine simulation by `dt_ms` (typically called at 50 Hz, dt = 20 ms).
    ///
    /// - `throttle`: User throttle demand in [0, 100].
    /// - `jake_brake`: Whether compression release braking is actively engaged.
    pub fn update(&mut self, throttle: u8, jake_brake: bool, dt_ms: u16) {
        match self.state {
            EngineState::Off => {
                self.current_rpm = 0.0;
                self.pitch_ratio = 1.0;
            }
            EngineState::Starting => {
                self.state_timer_ms += dt_ms;
                if self.state_timer_ms >= self.config.start_duration_ms {
                    self.state = EngineState::Running;
                    self.current_rpm = self.config.idle_rpm as f32;
                    self.state_timer_ms = 0;
                }
            }
            EngineState::Running => {
                let idle = self.config.idle_rpm as f32;
                let max = self.config.max_rpm as f32;

                if jake_brake {
                    // Jake brake rapidly forces RPM down towards idle
                    let decel = (self.config.jake_brake_decel_rate as f32) * 5.0;
                    self.current_rpm = (self.current_rpm - decel).max(idle);
                } else {
                    // Compute target RPM from throttle
                    let throttle_norm = (throttle.min(100) as f32) / 100.0;
                    self.target_rpm = idle + (max - idle) * throttle_norm;

                    // Apply flywheel inertia smoothing
                    let inertia = (self.config.inertia.max(1) as f32) * 2.0;
                    let diff = self.target_rpm - self.current_rpm;
                    self.current_rpm += diff / inertia;
                }

                self.current_rpm = self.current_rpm.clamp(self.config.min_rpm as f32, max);

                // Compute pitch ratio relative to idle
                let raw_pitch = self.current_rpm / idle;
                self.pitch_ratio = raw_pitch.min(self.config.max_pitch_factor);
            }
            EngineState::Stopping => {
                self.state_timer_ms += dt_ms;
                let progress = (self.state_timer_ms as f32) / (self.config.stop_duration_ms as f32);
                let idle = self.config.idle_rpm as f32;
                self.current_rpm = (idle * (1.0 - progress)).max(0.0);

                if self.state_timer_ms >= self.config.stop_duration_ms {
                    self.state = EngineState::Off;
                    self.current_rpm = 0.0;
                }
            }
            EngineState::ParkingBrake => {
                self.current_rpm = self.config.idle_rpm as f32;
                self.pitch_ratio = 1.0;
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_engine_start_cycle() {
        let mut sim = EngineSim::default();
        assert_eq!(sim.state, EngineState::Off);

        sim.start();
        assert_eq!(sim.state, EngineState::Starting);

        // Advance 1200ms in 20ms steps
        for _ in 0..60 {
            sim.update(0, false, 20);
        }

        assert_eq!(sim.state, EngineState::Running);
        assert_eq!(sim.current_rpm as u16, sim.config.idle_rpm);
        assert!((sim.pitch_ratio - 1.0).abs() < 1e-4);
    }

    #[test]
    fn test_throttle_rev_ramp() {
        let mut sim = EngineSim::default();
        sim.start();
        for _ in 0..60 {
            sim.update(0, false, 20);
        }

        // Apply 100% throttle
        for _ in 0..50 {
            sim.update(100, false, 20);
        }

        // RPM should have accelerated towards max_rpm
        assert!(sim.current_rpm > 2000.0);
        assert!(sim.pitch_ratio > 3.0);
    }
}
