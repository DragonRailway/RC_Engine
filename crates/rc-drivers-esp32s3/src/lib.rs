//! Hardware drivers for ESP32-S3 RC Engine boards.
//!
//! Provides clean, reusable abstractions for:
//! - Power latch control and long-press shutdown state machine
//! - Calibrated battery voltage sensing
//! - Ultrasonic 20 kHz dual-PWM motor drive
//! - Dual-core continuous I2S DMA audio pipeline (Core 1)
//! - `BoardHardware` bundle passed to the central runtime

#![no_std]

use bsp_core::{Board, DynamicPinPool, ResistorDivider};

/// Power state machine tracking button presses and latch hold.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum PowerState {
    #[default]
    Starting,
    Running,
    PoweringDown,
}

/// Power management controller managing `PWR_EN` and `PWR_BTN`.
#[derive(Debug, Clone, Copy)]
pub struct PowerController {
    pub state: PowerState,
    pub power_latch_pin: Option<u8>,
    pub power_button_pin: Option<u8>,
    button_hold_timer_ms: u16,
    shutdown_threshold_ms: u16,
}

impl PowerController {
    pub const fn new(power_latch_pin: Option<u8>, power_button_pin: Option<u8>) -> Self {
        Self {
            state: PowerState::Starting,
            power_latch_pin,
            power_button_pin,
            button_hold_timer_ms: 0,
            shutdown_threshold_ms: 3000, // 3 second long-press to power off
        }
    }

    /// Step the power controller state machine (called at 50 Hz, dt = 20 ms).
    ///
    /// - `button_pressed`: True if power button is currently held down.
    ///
    /// Returns `true` if the board should immediately cut power latch.
    pub fn update(&mut self, button_pressed: bool, dt_ms: u16) -> bool {
        match self.state {
            PowerState::Starting => {
                // Immediate latch activation
                self.state = PowerState::Running;
                false
            }
            PowerState::Running => {
                if button_pressed {
                    self.button_hold_timer_ms = self.button_hold_timer_ms.saturating_add(dt_ms);
                    if self.button_hold_timer_ms >= self.shutdown_threshold_ms {
                        self.state = PowerState::PoweringDown;
                        return true; // Trigger shutdown
                    }
                } else {
                    self.button_hold_timer_ms = 0;
                }
                false
            }
            PowerState::PoweringDown => true,
        }
    }
}

/// Calibrated Battery ADC reader applying the board's exact resistor divider ratio.
#[derive(Debug, Clone, Copy)]
pub struct BatteryAdcDriver {
    pub divider: ResistorDivider,
    pub adc_pin: Option<u8>,
    pub reference_voltage: f32, // Typically 1.1V or 3.1V depending on attenuation
}

impl BatteryAdcDriver {
    pub const fn new(divider: ResistorDivider, adc_pin: Option<u8>) -> Self {
        Self {
            divider,
            adc_pin,
            reference_voltage: 3.1, // 11 dB attenuation default
        }
    }

    /// Converts raw 12-bit ADC reading (0..4095) to true battery voltage.
    #[inline]
    pub fn raw_to_battery_voltage(&self, raw_adc: u16) -> f32 {
        let pin_volts = (raw_adc.min(4095) as f32 / 4095.0) * self.reference_voltage;
        self.divider.adc_to_battery_volts(pin_volts)
    }
}

/// Ultrasonic 20 kHz Dual-PWM motor output mapping.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct PwmDuty {
    pub duty_ch1: u16, // 0..1023 (10-bit resolution)
    pub duty_ch2: u16,
}

impl PwmDuty {
    /// Compute dual-PWM duty cycles from a signed speed (-100 to +100).
    ///
    /// Applies deadtime and direction swapping:
    /// - Speed > 0: ch1 = PWM, ch2 = 0 (Forward)
    /// - Speed < 0: ch1 = 0, ch2 = PWM (Reverse)
    /// - Speed == 0: ch1 = 0, ch2 = 0 (Coast / Brake depending on mode)
    pub fn from_speed(speed: i16, max_duty: u16) -> Self {
        let speed_clamped = speed.clamp(-100, 100);
        let magnitude = (speed_clamped.abs() as u32 * max_duty as u32 / 100) as u16;

        if speed_clamped > 0 {
            Self {
                duty_ch1: magnitude,
                duty_ch2: 0,
            }
        } else if speed_clamped < 0 {
            Self {
                duty_ch1: 0,
                duty_ch2: magnitude,
            }
        } else {
            Self {
                duty_ch1: 0,
                duty_ch2: 0,
            }
        }
    }
}

/// Core 1 continuous I2S DMA streaming buffer ring configuration.
#[derive(Debug, Clone, Copy)]
pub struct I2sDmaConfig {
    pub sample_rate: u32,
    pub buffer_count: usize,
    pub buffer_frames: usize,
    pub lrc_pin: u8,
    pub bclk_pin: u8,
    pub din_pin: u8,
    pub sd_pin: Option<u8>,
}

impl I2sDmaConfig {
    pub const fn new(lrc: u8, bclk: u8, din: u8, sd: Option<u8>) -> Self {
        Self {
            sample_rate: 22050,
            buffer_count: 4,
            buffer_frames: 128, // 128 frames (~5.8 ms per DMA block)
            lrc_pin: lrc,
            bclk_pin: bclk,
            din_pin: din,
            sd_pin: sd,
        }
    }
}

/// Unified hardware structure handed off by the BSP to the central runtime.
#[derive(Debug, Clone)]
pub struct BoardHardware {
    pub name: &'static str,
    pub power: PowerController,
    pub battery_adc: BatteryAdcDriver,
    pub audio: I2sDmaConfig,
    pub dynamic_pins: DynamicPinPool,
}

impl BoardHardware {
    /// Constructs a standard `BoardHardware` bundle for any board implementing `Board`.
    pub fn from_board<B: Board>() -> Self {
        Self {
            name: B::NAME,
            power: PowerController::new(B::POWER.power_enable, B::POWER.power_button),
            battery_adc: BatteryAdcDriver::new(B::VOLTAGE_DIVIDER, B::POWER.voltage_sens),
            audio: I2sDmaConfig::new(B::AUDIO.lrc, B::AUDIO.bclk, B::AUDIO.din, B::AUDIO.sd),
            dynamic_pins: DynamicPinPool::new(),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_power_latch_state_machine() {
        let mut pwr = PowerController::new(Some(35), Some(36));
        assert_eq!(pwr.state, PowerState::Starting);

        let shutdown = pwr.update(false, 20);
        assert!(!shutdown);
        assert_eq!(pwr.state, PowerState::Running);

        for _ in 0..25 {
            assert!(!pwr.update(true, 20));
        }
        assert!(!pwr.update(false, 20));

        for _ in 0..149 {
            assert!(!pwr.update(true, 20));
        }
        assert!(pwr.update(true, 20));
        assert_eq!(pwr.state, PowerState::PoweringDown);
    }

    #[test]
    fn test_battery_adc_scaling() {
        let driver = BatteryAdcDriver::new(ResistorDivider::new(100.0, 100.0), Some(2));
        let volts = driver.raw_to_battery_voltage(2048);
        assert!((volts - 3.10).abs() < 0.05);

        let volts_full = driver.raw_to_battery_voltage(4095);
        assert!((volts_full - 6.20).abs() < 0.01);
    }

    #[test]
    fn test_pwm_duty_dual_pwm() {
        let max_duty = 1023u16;

        let duty_fwd = PwmDuty::from_speed(80, max_duty);
        assert_eq!(duty_fwd.duty_ch1, 818);
        assert_eq!(duty_fwd.duty_ch2, 0);

        let duty_rev = PwmDuty::from_speed(-50, max_duty);
        assert_eq!(duty_rev.duty_ch1, 0);
        assert_eq!(duty_rev.duty_ch2, 511);

        let duty_stop = PwmDuty::from_speed(0, max_duty);
        assert_eq!(duty_stop.duty_ch1, 0);
        assert_eq!(duty_stop.duty_ch2, 0);
    }
}
