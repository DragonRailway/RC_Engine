## ADDED Requirements

### Requirement: CPU Frequency-Agnostic MCPWM Timing
The MCPWM peripheral subsystem (used for steering servos, ESC PPM pulses, and DC motor drivers) SHALL dynamically compensate its timer configuration across CPU frequencies (including 80 MHz, 160 MHz, and 240 MHz on ESP32-S3 and classic ESP32) so that timer tick resolutions, servo pulse widths (1000–2000 µs), servo frame rates (50 Hz / 20 ms period), and motor ultrasonic frequencies (20 kHz) remain accurate regardless of CPU clock frequency scaling.

#### Scenario: 80 MHz CPU servo pulse accuracy
- **WHEN** the CPU frequency is configured at 80 MHz (`board_build.f_cpu = 80000000L`)
- **THEN** a commanded 1500 µs neutral servo pulse outputs a hardware pulse width of 1500 µs ± 1 µs at a 50 Hz frame rate (20 ms period)

#### Scenario: 80 MHz CPU motor PWM frequency
- **WHEN** the CPU frequency is configured at 80 MHz
- **THEN** an `EasyMotor` configured for 20 kHz ultrasonic operation generates a PWM carrier frequency of 20 kHz ± 100 Hz

#### Scenario: 160 MHz and 240 MHz backward compatibility
- **WHEN** the CPU frequency is configured at standard 160 MHz or 240 MHz
- **THEN** servo PPM pulses and motor PWM carrier frequencies continue to match commanded values without drift or distortion
