//! Sensors and expanders for the RK_ULTRA flagship board.
//!
//! Provides:
//! - LSM6DSVTR 6-axis IMU (SPI) accelerometer & gyroscope data parsing
//! - AW9523 16-channel LED expander (I2C) constant-current brightness control

#![no_std]

/// 3-axis accelerometer and gyroscope measurement.
#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct ImuReading {
    pub accel_x_g: f32,
    pub accel_y_g: f32,
    pub accel_z_g: f32,
    pub gyro_x_dps: f32,
    pub gyro_y_dps: f32,
    pub gyro_z_dps: f32,
}

impl ImuReading {
    /// Compute vehicle roll angle in degrees from gravity vector.
    pub fn roll_deg(&self) -> f32 {
        libm::atan2f(self.accel_y_g, self.accel_z_g) * (180.0 / core::f32::consts::PI)
    }

    /// Compute vehicle pitch angle in degrees from gravity vector.
    pub fn pitch_deg(&self) -> f32 {
        let norm = libm::sqrtf(self.accel_y_g * self.accel_y_g + self.accel_z_g * self.accel_z_g);
        -libm::atan2f(self.accel_x_g, norm) * (180.0 / core::f32::consts::PI)
    }
}

/// AW9523 16-channel I2C constant-current LED driver commands.
pub struct Aw9523Command;

impl Aw9523Command {
    pub const I2C_ADDR: u8 = 0x5B;

    /// Registers for dimming channels 0 through 15 (0x20..0x2F).
    pub const fn dim_reg(channel: u8) -> u8 {
        0x20 + (channel & 0x0F)
    }

    /// Format an I2C transaction to set channel brightness (0..255).
    pub fn set_channel_dim(channel: u8, brightness: u8) -> [u8; 2] {
        [Self::dim_reg(channel), brightness]
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_imu_orientation_math() {
        // Flat vehicle: 1G pointing down Z
        let flat = ImuReading {
            accel_x_g: 0.0,
            accel_y_g: 0.0,
            accel_z_g: 1.0,
            ..Default::default()
        };
        assert!(flat.roll_deg().abs() < 1e-3);
        assert!(flat.pitch_deg().abs() < 1e-3);

        // Vehicle tilted 45 degrees roll
        let tilt = ImuReading {
            accel_x_g: 0.0,
            accel_y_g: 0.7071,
            accel_z_g: 0.7071,
            ..Default::default()
        };
        assert!((tilt.roll_deg() - 45.0).abs() < 0.1);
    }

    #[test]
    fn test_aw9523_commands() {
        let cmd0 = Aw9523Command::set_channel_dim(0, 128);
        assert_eq!(cmd0, [0x20, 128]);

        let cmd15 = Aw9523Command::set_channel_dim(15, 255);
        assert_eq!(cmd15, [0x2F, 255]);
    }
}
