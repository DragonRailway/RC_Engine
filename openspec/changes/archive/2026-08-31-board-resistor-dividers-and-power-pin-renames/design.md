# Design: Board Resistor Dividers & Power Pin Renaming

## Context
ADC battery voltage measurement requires converting measured pin voltage ($V_{\text{pin}}$) to actual battery pack voltage ($V_{\text{bat}}$) based on the board's resistor divider circuit ($V_{\text{pin}} = V_{\text{bat}} \times \frac{R_{\text{low}}}{R_{\text{high}} + R_{\text{low}}}$).
Historically, the firmware required an arbitrary `voltage_scale` multiplier in every hardware JSON config or relied on compile-time `-D VSCALE=...` build flags.

Additionally, board power management pins on accessory-capable boards (such as `GTRACK`) used legacy names (`BUCK_5V_EN`, `POWER_OUT`) that did not clearly reflect their semantic functions (`SERVO_ENABLE` and `PUMP_ENABLE`).

## Goals / Non-Goals

**Goals:**
- Reflect exact schematic resistor values in `boards/*.h` via `VOLTAGE_DIV_R_HIGH` and `VOLTAGE_DIV_R_LOW`.
- Automatically calculate `POWER::DIVIDER_RATIO` at compile time with a fallback to `2.0` (1:1 divider) if resistors are omitted for a configured voltage sense pin.
- Provide clean, optional `calibration_factor` (default `1.0`) and `voltage_offset` (default `0.0`) in hardware JSON configs for per-unit ADC trim.
- Rename `BUCK_5V_EN` to `SERVO_ENABLE` and `POWER_OUT` to `PUMP_ENABLE` across board headers, `BoardBase`, `HardwareInit`, and docs.
- Remove legacy compile-time `VSCALE` / `VOFFSET` macro flags from `platformio.ini`.

**Non-Goals:**
- Retaining legacy `VSCALE` compile macros (explicitly not needed).

## Decisions

### Decision 1: Dimensionless Resistor Constants in `boards/*.h`
Instead of specifying fixed units in variable names (e.g. `_KOHM`), we define:
```cpp
struct POWER {
    static constexpr uint8_t VOLTAGE_SENS       = 7;
    static constexpr float   VOLTAGE_DIV_R_HIGH = 20.0f;
    static constexpr float   VOLTAGE_DIV_R_LOW  = 5.1f;
    static constexpr float   DIVIDER_RATIO      = BoardBase::computeVoltageDividerRatio(VOLTAGE_DIV_R_HIGH, VOLTAGE_DIV_R_LOW, VOLTAGE_SENS);
    static constexpr uint8_t SERVO_ENABLE       = 44;   // 5V buck for servos
    static constexpr uint8_t PUMP_ENABLE        = 43;   // High-side MOSFET for hydraulic pump
};
```
*Rationale*: Since the divider ratio is dimensionless ($(R_{\text{high}} + R_{\text{low}}) / R_{\text{low}}$), any uniform unit (kΩ or Ω) yields the exact same ratio.

### Decision 2: Constexpr Fallback Rule in `BoardBase`
In `boards/BoardBase.h`:
```cpp
static constexpr float computeVoltageDividerRatio(float rHigh, float rLow, uint8_t pin = 0) {
    if (pin == 0xFF) return 0.0f;
    if (rLow > 0.0f) {
        return (rHigh + rLow) / rLow;
    }
    return 2.0f; // Standard 1:1 divider fallback
}
```
*Rationale*: If a board author defines `VOLTAGE_SENS` but forgets or omits explicit resistor values, the system safely defaults to a 1:1 (ratio 2.0) divider.

### Decision 3: Optional `calibration_factor` in JSON Config
In `HardwareConfig::Battery` and `ConfigParser.cpp`:
- `vScale = BOARD::POWER::DIVIDER_RATIO * (batObj["calibration_factor"] | 1.0f);`
- `vOffset = batObj["voltage_offset"] | 0.0f;`

*Rationale*: Clean configs omit `calibration_factor` and `voltage_scale` entirely, using the physical board ratio. Units with measured ADC error can add `"calibration_factor": 0.985`.

### Decision 4: Power Pin Renames
- `BUCK_5V_EN` $\rightarrow$ `SERVO_ENABLE`
- `POWER_OUT` $\rightarrow$ `PUMP_ENABLE`

*Rationale*: Accurately describes the connected subsystem function.

## Risks / Trade-offs

- **[Division by zero in ratio calculation]** $\rightarrow$ Mitigated by guarding `rLow > 0.0f` in `computeVoltageDividerRatio`.
- **[Host unit tests without board headers]** $\rightarrow$ `test/host_vc/host_vc_driver.cpp` provides a mock `vScale` in `testHw.battery.vScale = 2.0f`.
