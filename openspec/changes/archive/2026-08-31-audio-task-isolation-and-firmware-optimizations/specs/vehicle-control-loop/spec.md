## ADDED Requirements

### Requirement: Decimated 5 Hz battery voltage ADC sampling
The vehicle control loop SHALL decimate battery voltage ADC acquisition from 50 Hz to 5 Hz (every 10 control ticks / 200 ms):
1. `HardwareInit::readBatteryVoltage()` SHALL be invoked once every 10 control loop iterations (200 ms).
2. The sampled voltage SHALL be smoothed using an exponential moving average (EMA) low-pass filter ($\alpha = 0.1$).
3. Battery warning and cutoff thresholds SHALL be evaluated against the smoothed voltage.

#### Scenario: High motor acceleration load
- **WHEN** full throttle causes high-current motor switching noise on the power rails
- **THEN** battery voltage sampling filters out transient switching ripple and avoids false low-voltage cutoffs
