## MODIFIED Requirements

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
