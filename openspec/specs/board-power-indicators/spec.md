# board-power-indicators Specification

## Purpose
TBD - created by archiving change board-power-indicators. Update Purpose after archive.
## Requirements
### Requirement: Configurable charging section and indicator output

The hardware configuration schema (`configs/schemas/hardware_config.schema.json`) SHALL support a top-level `"charging"` configuration object:
- `hardware`: string, pin token (e.g. `L1`) or light alias (e.g. `head_light`, `cab_light`, `tail_light`, `brake_light`, `step_light`, `aux_light`)
- `mode`: string, enum `["solid", "blink", "pulse", "auto"]`, default `"auto"`

Charging Indicator Behavior:
1. **Active Charging State (`CHARGING_ACTIVE`)**: The configured charging indicator SHALL animate a heartbeat double-pulse rhythm:
   - 0ms – 120ms: ON at 100% of configured `brightness_max`
   - 120ms – 240ms: OFF
   - 240ms – 360ms: ON at 100% of configured `brightness_max`
   - 360ms – 1000ms: OFF
2. **Charge Complete State (`CHARGING_COMPLETE`)**: The configured charging indicator SHALL switch to steady solid illumination at 20% of configured `brightness_max`.
3. If no dedicated charging indicator is configured (`0xFF`), the headlight SHALL serve as the fallback indicator.

#### Scenario: Configured charging indicator pin and mode
- **WHEN** hardware config specifies `"charging": { "hardware": "head_light" }`
- **THEN** `ConfigParser` resolves the headlight pin for charging indicator and activates the default charging indicator behavior

#### Scenario: Heartbeat animation during active charging
- **WHEN** board is in `CHARGING_ACTIVE` state
- **THEN** charging indicator executes a 1-second repeating heartbeat double-pulse at 100% max brightness

#### Scenario: Solid 20% brightness when charge finishes
- **WHEN** board transitions to `CHARGING_COMPLETE`
- **THEN** charging indicator transitions to steady solid output at 20% of maximum brightness

---

### Requirement: Configurable power indicator output and button-hold visual feedback

The hardware configuration schema SHALL support an optional `"hardware"` property in the `"power"` configuration object:
- `hardware`: string, pin token or light alias

During a physical power button hold:
1. While `POWER_BUTTON` is held (`HIGH`), the power indicator SHALL blink rapidly (200ms interval) for the duration of `button_hold_s`.
2. When `button_hold_s` elapses, the indicator SHALL switch OFF completely as `HardwareInit::powerOff()` executes.
3. If released before `button_hold_s`, normal light states SHALL be restored.

#### Scenario: Rapid blink feedback during power button hold
- **WHEN** user presses and holds `POWER_BUTTON`
- **THEN** power indicator blinks at 200ms intervals during `button_hold_s` countdown
- **WHEN** `button_hold_s` elapses
- **THEN** power indicator turns OFF and system powers off

---

### Requirement: Disconnect warning visual alert via power indicator

During the 10-second disconnect warning phase, the system SHALL drive visual warning alerts:
1. If a power indicator is configured, it SHALL blink a warning pattern alongside the audio alert tone.
2. If no power indicator is configured (`0xFF`), hazard turn indicators SHALL blink as fallback.

#### Scenario: Visual disconnect warning pattern
- **WHEN** disconnect timer enters the final 10 seconds (`warning_window_s`)
- **THEN** power indicator (or fallback hazard indicators) blinks warning pattern

