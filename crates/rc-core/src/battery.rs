//! LiPo battery voltage monitoring, cell auto-detection, and low-voltage safety cutoff.

/// LiPo battery health and safety state.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum BatteryState {
    #[default]
    Normal,
    Warning, // Approaching cutoff threshold (trigger audio beep / blinking lights)
    Cutoff,  // Critical low voltage (shut down motor throttle to prevent cell damage)
}

/// LiPo battery safety monitor.
pub struct BatteryMonitor {
    pub cell_count: u8,
    pub warning_voltage_per_cell: f32,
    pub cutoff_voltage_per_cell: f32,
    pub filtered_voltage: f32,
    pub state: BatteryState,
    low_voltage_time_ms: u16,
}

impl Default for BatteryMonitor {
    fn default() -> Self {
        Self::new(3.4, 3.2)
    }
}

impl BatteryMonitor {
    pub const fn new(warning_per_cell: f32, cutoff_per_cell: f32) -> Self {
        Self {
            cell_count: 2, // Default to 2S
            warning_voltage_per_cell: warning_per_cell,
            cutoff_voltage_per_cell: cutoff_per_cell,
            filtered_voltage: 0.0,
            state: BatteryState::Normal,
            low_voltage_time_ms: 0,
        }
    }

    /// Auto-detect cell count (1S–4S) from initial boot voltage.
    pub fn auto_detect_cells(&mut self, initial_voltage: f32) {
        self.filtered_voltage = initial_voltage;

        self.cell_count = if initial_voltage > 13.0 {
            4
        } else if initial_voltage > 9.0 {
            3
        } else if initial_voltage > 5.0 {
            2
        } else {
            1
        };
    }

    /// Update with a new raw voltage reading.
    ///
    /// Applies exponential moving average (IIR filter) to reject motor noise and dips.
    pub fn update(&mut self, raw_voltage: f32, dt_ms: u16) -> BatteryState {
        // Low-pass filter (alpha = 0.1)
        if self.filtered_voltage <= 0.01 {
            self.filtered_voltage = raw_voltage;
        } else {
            self.filtered_voltage = 0.9 * self.filtered_voltage + 0.1 * raw_voltage;
        }

        let cells = (self.cell_count as f32).max(1.0);
        let voltage_per_cell = self.filtered_voltage / cells;

        if voltage_per_cell <= self.cutoff_voltage_per_cell {
            self.low_voltage_time_ms = self.low_voltage_time_ms.saturating_add(dt_ms);
            // Debounce: require 1.0 second of continuous low voltage before cut
            if self.low_voltage_time_ms >= 1000 {
                self.state = BatteryState::Cutoff;
            } else {
                self.state = BatteryState::Warning;
            }
        } else if voltage_per_cell <= self.warning_voltage_per_cell {
            self.state = BatteryState::Warning;
            self.low_voltage_time_ms = 0;
        } else {
            self.state = BatteryState::Normal;
            self.low_voltage_time_ms = 0;
        }

        self.state
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_cell_detection() {
        let mut bat = BatteryMonitor::default();

        bat.auto_detect_cells(8.2); // Fully charged 2S
        assert_eq!(bat.cell_count, 2);

        bat.auto_detect_cells(12.4); // Fully charged 3S
        assert_eq!(bat.cell_count, 3);
    }

    #[test]
    fn test_low_voltage_cutoff_debouncing() {
        let mut bat = BatteryMonitor::default();
        bat.auto_detect_cells(6.2); // Already low 2S (3.1V/cell < 3.2V cutoff)

        // Drop below cutoff immediately
        for _ in 0..20 {
            // 20 steps of 20ms = 400ms (debouncing, warning state)
            bat.update(6.0, 20);
        }
        assert_eq!(bat.state, BatteryState::Warning);

        for _ in 0..40 {
            // Another 40 steps = total 1200ms (> 1000ms debounce)
            bat.update(6.0, 20);
        }
        assert_eq!(bat.state, BatteryState::Cutoff);
    }
}
