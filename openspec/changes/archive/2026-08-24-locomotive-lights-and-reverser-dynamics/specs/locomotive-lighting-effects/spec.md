## ADDED Requirements

### Requirement: Incandescent Bulb Soft PWM Fade
The firmware SHALL apply asymmetric software slew-rate ramping to locomotive lighting channels (`head_light`, `tail_light`, `cab_light`, `step_light`) to emulate incandescent bulb filament warm-up and cool-down.

#### Scenario: Light channel toggled on
- **WHEN** a locomotive light channel is commanded ON
- **THEN** output PWM duty ramps up gradually to the target brightness over a fade-in duration (~200ms)

#### Scenario: Light channel toggled off
- **WHEN** a locomotive light channel is commanded OFF
- **THEN** output PWM duty ramps down to zero over a fast cool-down duration (~100ms)

### Requirement: Ditch Light Triangular Cross-Fade
The locomotive controller SHALL alternate dual ditch lights (`left` / `L4` and `right` / `L5`) using a continuous triangular/sinusoidal cross-fade curve when Ditch Lights are active.

#### Scenario: Ditch lights oscillating
- **WHEN** ditch lights are active (via `loco_light` Bit 3, or automated horn/bell trigger)
- **THEN** left fixture brightness ramps up while right fixture brightness ramps down inversely
- **AND** the cycle reverses smoothly when reaching duty boundaries
