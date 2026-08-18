# Board Power Control & Battery Protection

## ADDED Requirements

### Requirement: Boot power latching with 1000ms threshold filter

The firmware SHALL configure `POWER_ENABLE` as an output and `POWER_BUTTON` as an active-HIGH input pin at startup. At boot time in `setup()`, the system SHALL wait for `POWER_BUTTON` to be held HIGH for at least 1000ms before driving `POWER_ENABLE` HIGH to latch hardware power. If `POWER_BUTTON` is released before 1000ms has elapsed, the firmware SHALL NOT drive `POWER_ENABLE` HIGH.

#### Scenario: Valid power-on hold (>= 1000ms)
- **WHEN** `POWER_BUTTON` is held HIGH for 1000ms or longer during boot
- **THEN** `POWER_ENABLE` is set HIGH, latching board power ON

#### Scenario: Accidental brief touch (< 1000ms)
- **WHEN** `POWER_BUTTON` is released before 1000ms has elapsed during boot
- **THEN** `POWER_ENABLE` remains LOW, causing board power to turn OFF upon physical button release

### Requirement: Runtime power button monitoring & 4000ms long-press shutdown

While running, `HardwareInit` / `VehicleController` SHALL monitor `POWER_BUTTON` state each main loop iteration. If `POWER_BUTTON` is held HIGH continuously for 4000ms or longer, the firmware SHALL perform a graceful shutdown (stopping all motor channels and audio output) and drive `POWER_ENABLE` LOW to turn off hardware board power. Short button presses (< 4000ms) SHALL be ignored.

#### Scenario: Button held for 4000ms at runtime
- **WHEN** `POWER_BUTTON` is held HIGH continuously for >= 4000ms while the vehicle is running
- **THEN** all outputs are stopped, audio is halted, and `POWER_ENABLE` is set LOW to power off the board

#### Scenario: Short button press at runtime
- **WHEN** `POWER_BUTTON` is pressed and released in under 4000ms
- **THEN** the timer resets and no power-off or state change occurs

### Requirement: Two-tier battery warning and 3.3V cutoff protection

The firmware SHALL implement a two-tier battery protection system based on measured pack voltage against configured per-cell thresholds (`warning_voltage` default 3.5V/cell, `cutoff_voltage` default 3.3V/cell).

#### Scenario: Battery drops below warning voltage
- **WHEN** pack voltage drops below `cellCount × warning_voltage` (3.5V/cell)
- **THEN** the system triggers a low-voltage warning alert state (audio/telemetry) while motor drive remains enabled

#### Scenario: Battery drops below cutoff voltage
- **WHEN** pack voltage drops below `cellCount × cutoff_voltage` (3.3V/cell) for 1500ms
- **THEN** motor drive is disabled, audio is stopped, and `POWER_ENABLE` is driven LOW to power off hardware power completely

### Requirement: Hardware config schema for battery thresholds

The hardware configuration schema (`configs/schemas/hardware_config.schema.json`) SHALL support `warning_voltage` (default 3.5V, range 3.0–4.0V) and `cutoff_voltage` (default 3.3V, range 3.0–3.8V) in the `battery` object.

#### Scenario: Hardware config with warning and cutoff voltages
- **WHEN** hardware config specifies `cell_count: 1`, `warning_voltage: 3.5`, `cutoff_voltage: 3.3`
- **THEN** `ConfigParser` populates `HardwareConfig::battery` with warning=3.5V and cutoff=3.3V
