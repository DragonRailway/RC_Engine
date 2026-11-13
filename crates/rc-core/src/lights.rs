//! Lighting automation, sequencers, and safety indicators.

/// Headlight 3-stage mode.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum HeadlightMode {
    #[default]
    Off,
    LowBeam,
    HighBeam,
}

/// Turn indicator state.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum TurnMode {
    #[default]
    Off,
    Left,
    Right,
    Hazard,
}

/// Output brightness percentages (0 to 100) for all lighting channels.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct LightOutputs {
    pub headlights: u8,
    pub high_beams: u8,
    pub tail_lights: u8,
    pub brake_lights: u8,
    pub turn_left: u8,
    pub turn_right: u8,
    pub reversing_light: u8,
    pub cabin_light: u8,
    pub beacon_light: u8,
}

/// Lighting automation controller.
pub struct LightingSequencer {
    pub headlight_mode: HeadlightMode,
    pub turn_mode: TurnMode,
    pub cabin_active: bool,
    pub beacon_active: bool,

    // Turn signal auto-cancellation
    left_armed: bool,
    right_armed: bool,

    // Flasher timers
    flasher_timer_ms: u16,
    flasher_state: bool,

    // Brake light timer
    brake_timer_ms: u16,
    prev_throttle: i16,
}

impl Default for LightingSequencer {
    fn default() -> Self {
        Self::new()
    }
}

impl LightingSequencer {
    pub const fn new() -> Self {
        Self {
            headlight_mode: HeadlightMode::Off,
            turn_mode: TurnMode::Off,
            cabin_active: false,
            beacon_active: false,
            left_armed: false,
            right_armed: false,
            flasher_timer_ms: 0,
            flasher_state: false,
            brake_timer_ms: 0,
            prev_throttle: 0,
        }
    }

    /// Advance lighting sequencer and compute channel brightness percentages.
    pub fn update(&mut self, throttle: i16, steering: i16, dt_ms: u16) -> LightOutputs {
        // 1. Flasher timer (500 ms cycle)
        self.flasher_timer_ms += dt_ms;
        if self.flasher_timer_ms >= 500 {
            self.flasher_timer_ms = 0;
            self.flasher_state = !self.flasher_state;
        }

        // 2. Auto turn-signal cancel on steering wheel return
        if steering < -30 {
            self.left_armed = true;
        } else if steering > 30 {
            self.right_armed = true;
        }

        if self.left_armed && steering > -10 && self.turn_mode == TurnMode::Left {
            self.turn_mode = TurnMode::Off;
            self.left_armed = false;
        }
        if self.right_armed && steering < 10 && self.turn_mode == TurnMode::Right {
            self.turn_mode = TurnMode::Off;
            self.right_armed = false;
        }

        // 3. Dynamic deceleration brake light trigger
        let throttle_drop = self.prev_throttle - throttle;
        if throttle_drop > 25 || (self.prev_throttle != 0 && throttle == 0) {
            self.brake_timer_ms = 1200; // Hold brake light for 1.2s
        }
        if self.brake_timer_ms > 0 {
            self.brake_timer_ms = self.brake_timer_ms.saturating_sub(dt_ms);
        }
        self.prev_throttle = throttle;

        // 4. Compute channel brightnesses
        let (headlights, high_beams) = match self.headlight_mode {
            HeadlightMode::Off => (0, 0),
            HeadlightMode::LowBeam => (40, 0),
            HeadlightMode::HighBeam => (100, 100),
        };

        let is_braking = self.brake_timer_ms > 0;
        let tail_lights = if self.headlight_mode != HeadlightMode::Off {
            30
        } else {
            0
        };
        let brake_lights = if is_braking { 100 } else { tail_lights };

        let flasher_val = if self.flasher_state { 100 } else { 0 };
        let (turn_left, turn_right) = match self.turn_mode {
            TurnMode::Off => (0, 0),
            TurnMode::Left => (flasher_val, 0),
            TurnMode::Right => (0, flasher_val),
            TurnMode::Hazard => (flasher_val, flasher_val),
        };

        let reversing_light = if throttle < 0 { 100 } else { 0 };
        let cabin_light = if self.cabin_active { 100 } else { 0 };
        let beacon_light = if self.beacon_active && self.flasher_state {
            100
        } else {
            0
        };

        LightOutputs {
            headlights,
            high_beams,
            tail_lights,
            brake_lights,
            turn_left,
            turn_right,
            reversing_light,
            cabin_light,
            beacon_light,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_headlight_stepping() {
        let mut seq = LightingSequencer::new();

        seq.headlight_mode = HeadlightMode::Off;
        let out_off = seq.update(0, 0, 20);
        assert_eq!(out_off.headlights, 0);
        assert_eq!(out_off.high_beams, 0);

        seq.headlight_mode = HeadlightMode::LowBeam;
        let out_low = seq.update(0, 0, 20);
        assert_eq!(out_low.headlights, 40);
        assert_eq!(out_low.high_beams, 0);

        seq.headlight_mode = HeadlightMode::HighBeam;
        let out_high = seq.update(0, 0, 20);
        assert_eq!(out_high.headlights, 100);
        assert_eq!(out_high.high_beams, 100);
    }

    #[test]
    fn test_dynamic_brake_light_on_decel() {
        let mut seq = LightingSequencer::new();

        // Driving at steady 80% throttle
        seq.update(80, 0, 20);
        let out_steady = seq.update(80, 0, 20);
        assert_eq!(out_steady.brake_lights, 0);

        // Sharp drop from 80% to 20%
        let out_decel = seq.update(20, 0, 20);
        assert_eq!(out_decel.brake_lights, 100);
    }
}
