//! Vehicle motion control, steering, and motor throttle mixing.

/// Drivetrain kinematics mode.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum DrivetrainMode {
    #[default]
    Ackermann, // Single drive motor + front steering servo
    SkidSteer, // Dual differential motors (tracks or skid-steer wheels)
}

/// Output actuator command structure.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct MotorOutputs {
    /// Primary drive motor speed (-100 to +100).
    pub drive: i16,
    /// Front steering servo position (-100 to +100, 0 = center).
    pub steering: i16,
    /// Left track motor for skid-steer (-100 to +100).
    pub left_track: i16,
    /// Right track motor for skid-steer (-100 to +100).
    pub right_track: i16,
    /// Auxiliary machinery motor (-100 to +100).
    pub aux_motor: i16,
}

/// Vehicle motion controller mapping user joystick/pedal inputs to hardware outputs.
pub struct MotionController {
    pub mode: DrivetrainMode,
    pub deadband: u8,
    pub reverse_interlock: bool,
    current_direction: i8, // 1 = forward, -1 = reverse, 0 = stopped
}

impl Default for MotionController {
    fn default() -> Self {
        Self {
            mode: DrivetrainMode::Ackermann,
            deadband: 3,
            reverse_interlock: true,
            current_direction: 0,
        }
    }
}

impl MotionController {
    pub fn new(mode: DrivetrainMode) -> Self {
        Self {
            mode,
            ..Default::default()
        }
    }

    /// Process user input commands and compute actuator outputs.
    ///
    /// - `throttle`: Requested speed (-100 to +100, positive = forward).
    /// - `steering`: Requested steering (-100 to +100, positive = right).
    /// - `aux_slider`: Auxiliary channel input (-100 to +100).
    pub fn update(&mut self, throttle: i16, steering: i16, aux_slider: i16) -> MotorOutputs {
        // Apply deadband
        let th = if throttle.abs() < (self.deadband as i16) {
            0
        } else {
            throttle.clamp(-100, 100)
        };

        let st = if steering.abs() < (self.deadband as i16) {
            0
        } else {
            steering.clamp(-100, 100)
        };

        // Reverser direction tracking
        if th > 0 {
            self.current_direction = 1;
        } else if th < 0 {
            self.current_direction = -1;
        } else {
            self.current_direction = 0;
        }

        match self.mode {
            DrivetrainMode::Ackermann => MotorOutputs {
                drive: th,
                steering: st,
                left_track: 0,
                right_track: 0,
                aux_motor: aux_slider.clamp(-100, 100),
            },
            DrivetrainMode::SkidSteer => {
                // Differential mixing: left = th + st, right = th - st
                let left = (th + st).clamp(-100, 100);
                let right = (th - st).clamp(-100, 100);

                MotorOutputs {
                    drive: 0,
                    steering: 0,
                    left_track: left,
                    right_track: right,
                    aux_motor: aux_slider.clamp(-100, 100),
                }
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_ackermann_direct() {
        let mut ctrl = MotionController::new(DrivetrainMode::Ackermann);
        let out = ctrl.update(80, -30, 0);

        assert_eq!(out.drive, 80);
        assert_eq!(out.steering, -30);
    }

    #[test]
    fn test_skid_steer_mixing() {
        let mut ctrl = MotionController::new(DrivetrainMode::SkidSteer);

        // Straight forward
        let out_straight = ctrl.update(50, 0, 0);
        assert_eq!(out_straight.left_track, 50);
        assert_eq!(out_straight.right_track, 50);

        // Turn in place (spin right)
        let out_spin = ctrl.update(0, 50, 0);
        assert_eq!(out_spin.left_track, 50);
        assert_eq!(out_spin.right_track, -50);
    }
}
