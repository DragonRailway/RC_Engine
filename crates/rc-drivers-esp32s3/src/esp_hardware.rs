//! Concrete hardware driver bindings using `esp-hal` for ESP32-S3.
//!
//! Provides real peripheral handles for:
//! - Power latch GPIO Output & power button Input
//! - Dual-PWM H-bridge motor outputs using LEDC at 20 kHz ultrasonic frequency
//! - Battery voltage ADC measurement via ADC1 with calibration

use esp_hal::analog::adc::{Adc, AdcCalBasic, AdcChannel, AdcPin};
use esp_hal::gpio::{Input, Output};
use esp_hal::ledc::channel::{Channel, ChannelHW};
use esp_hal::ledc::LowSpeed;
use esp_hal::peripherals::ADC1;
use esp_hal::Blocking;

use crate::{BatteryAdcDriver, PowerController, PwmDuty};

/// Concrete ESP32-S3 Power Management driver using real GPIOs.
pub struct HardwarePower<'d> {
    pub controller: PowerController,
    pub latch_out: Option<Output<'d>>,
    pub button_in: Option<Input<'d>>,
}

impl<'d> HardwarePower<'d> {
    /// Creates and immediately asserts power latch pin HIGH to maintain board power.
    pub fn new(
        latch_out: Option<Output<'d>>,
        button_in: Option<Input<'d>>,
        power_latch_pin: Option<u8>,
        power_button_pin: Option<u8>,
    ) -> Self {
        let mut pwr = Self {
            controller: PowerController::new(power_latch_pin, power_button_pin),
            latch_out,
            button_in,
        };
        // Assert latch immediately on boot
        pwr.assert_latch(true);
        pwr
    }

    /// Set the physical power latch output pin.
    #[inline]
    pub fn assert_latch(&mut self, enabled: bool) {
        if let Some(ref mut pin) = self.latch_out {
            if enabled {
                pin.set_high();
            } else {
                pin.set_low();
            }
        }
    }

    /// Read whether the physical power button is currently pressed (active low).
    #[inline]
    pub fn is_button_pressed(&self) -> bool {
        if let Some(ref pin) = self.button_in {
            pin.is_low()
        } else {
            false
        }
    }

    /// Step the power controller state machine. If shutdown threshold is reached,
    /// deasserts the latch pin immediately.
    pub fn update(&mut self, dt_ms: u16) -> bool {
        let pressed = self.is_button_pressed();
        let shutdown = self.controller.update(pressed, dt_ms);
        if shutdown {
            self.assert_latch(false);
        }
        shutdown
    }
}

/// Concrete Dual-PWM motor output driver using ESP32-S3 LEDC at 20 kHz.
pub struct HardwareMotorPwm<'a> {
    pub channel_a: Channel<'a, LowSpeed>,
    pub channel_b: Channel<'a, LowSpeed>,
    pub max_duty_val: u32,
}

impl<'a> HardwareMotorPwm<'a> {
    /// Applies `PwmDuty` directly to LEDC hardware registers.
    pub fn apply_duty(&mut self, duty: PwmDuty) {
        // Map 0..1023 to 0..max_duty_val
        let duty_a = (duty.duty_ch1 as u32 * self.max_duty_val) / 1023;
        let duty_b = (duty.duty_ch2 as u32 * self.max_duty_val) / 1023;

        self.channel_a.set_duty_hw(duty_a);
        self.channel_b.set_duty_hw(duty_b);
    }
}

/// Concrete Battery ADC reader on ESP32-S3.
pub struct HardwareBatteryAdc<'d, PIN>
where
    PIN: AdcChannel,
{
    pub adc: Adc<'d, ADC1<'d>, Blocking>,
    pub pin: AdcPin<PIN, ADC1<'d>, AdcCalBasic<ADC1<'d>>>,
    pub driver: BatteryAdcDriver,
}

impl<'d, PIN> HardwareBatteryAdc<'d, PIN>
where
    PIN: AdcChannel,
{
    pub fn new(
        adc: Adc<'d, ADC1<'d>, Blocking>,
        pin: AdcPin<PIN, ADC1<'d>, AdcCalBasic<ADC1<'d>>>,
        driver: BatteryAdcDriver,
    ) -> Self {
        Self { adc, pin, driver }
    }

    /// Reads physical battery voltage in Volts.
    pub fn read_voltage(&mut self) -> Result<f32, ()> {
        let raw: u16 = nb::block!(self.adc.read_oneshot(&mut self.pin))?;
        Ok(self.driver.raw_to_battery_voltage(raw))
    }
}
