# EasyKit Animation Engine

## Purpose

Activate the vendored ESP32_EasyKit animation engines (servo easing, LED fades, blink patterns) that RC_brain previously left dormant: a main-loop `update()` pump, config-driven animation tunables, eased aux servos, faded headlight stepping, and blink timing driven by the hardware config.

## Requirements

### Requirement: Animation update pump
The firmware SHALL drive the ESP32_EasyKit animation engines by calling `update()` on every attached `EasyServo` and `EasyLED` every main-loop iteration, so easing moves, fades, and blink patterns advance non-blocking without any busy-waiting.

#### Scenario: Fade advances without blocking
- **WHEN** an LED fade is started and the main loop runs
- **THEN** the LED duty progresses from start to target across the configured duration while the rest of the control loop continues to run

#### Scenario: Servo easing advances without blocking
- **WHEN** an eased servo move is started and the main loop runs
- **THEN** the servo position progresses along the easing curve across the move duration while other outputs keep updating

### Requirement: Configurable aux-servo easing
Aux servos (dump bed, mixer, excavator arm) SHALL move through the eased `write(µs, speed, kIn, kOut)` path using the configured easing speed and strength, while the steering servo, drive motor, and ESC SHALL remain on instant writes.

#### Scenario: Eased aux servo move
- **WHEN** an aux-servo position is commanded while easing speed is above zero
- **THEN** the servo sweeps to the target over the duration implied by distance ÷ speed, with asymmetric sigmoid easing governed by `k_in`/`k_out`

#### Scenario: Steering stays instant
- **WHEN** the steering servo is commanded to a new position
- **THEN** the steering servo moves to the target immediately (no easing), regardless of animation settings

#### Scenario: Easing disabled
- **WHEN** the configured `easing_speed_deg_s` is 0
- **THEN** aux servos move instantly, matching the pre-change behavior

### Requirement: Headlight fade transitions
The 3-state headlight stepping (Off, Low Beam 40%, High Beam 100%) SHALL transition between states via a fade over the configured `fade_duration_ms` instead of snapping, and the tail light SHALL track the headlight's live brightness so it remains consistent during the transition.

#### Scenario: Headlight step fades
- **WHEN** the headlight widget steps the headlight to a new state
- **THEN** the headlight duty ramps from its current value to the new target over the configured fade duration using an ease-in-out curve

#### Scenario: Tail tracks live headlight brightness
- **WHEN** the headlight is mid-fade
- **THEN** the tail light brightness reflects the headlight's current (live) duty level rather than its final target

### Requirement: Config-driven turn-signal and hazard blink
Turn signals and hazard flashing SHALL use the EasyLED blink engine driven by the hardware config's `turn_light.interval_on`, `turn_light.interval_off`, and `brightness_max`, replacing any hardcoded flash timing, and no static duty writes SHALL target a pin while its blink engine owns it.

#### Scenario: Turn signal blinks at configured interval
- **WHEN** a turn indicator becomes active
- **THEN** its light blinks on for `interval_on` ms and off for `interval_off` ms at `brightness_max` duty until released

#### Scenario: Hazard blinks both sides at configured interval
- **WHEN** hazard mode is active
- **THEN** both turn indicators flash synchronously at the configured `interval_on`/`interval_off` interval

#### Scenario: Blink stops on release
- **WHEN** a turn indicator is released
- **THEN** the blink engine stops and the light is turned off

#### Scenario: No static writes during blink
- **WHEN** a turn indicator's blink engine is active
- **THEN** no other code path writes a static duty to that pin that would fight the blink pattern

### Requirement: Animation config schema
The hardware config SHALL accept an optional `lower_snake_case` `"animation"` block (`easing_speed_deg_s`, `easing_k_in`, `easing_k_out`, `fade_duration_ms`) that overrides firmware defaults, and SHALL use the defaults when the block is absent.

#### Scenario: Animation block parsed
- **WHEN** the hardware config contains an `"animation"` block
- **THEN** `ConfigParser` populates the `HardwareConfig` animation fields with the specified values

#### Scenario: Animation block absent
- **WHEN** the hardware config omits the `"animation"` block
- **THEN** the firmware uses the default easing speed, easing strengths, and fade duration
