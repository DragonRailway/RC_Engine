## Context

RadioKit provides an active header telemetry display for connected devices. In the current firmware:
- Battery percentage is calculated with a floor at the cutoff voltage (`3.3V/cell`), which means the UI displays ~20-25% remaining even when the battery has hit the low-voltage warning threshold (`3.5V/cell`).
- Speed telemetry transmits raw motor output percentage without conversion to realistic km/h values (0–200 km/h).

## Goals / Non-Goals

**Goals:**
- Scale battery percentage so 0% represents the configured `warning_voltage` per cell (clamped to 0% below warning), and 100% represents `full_voltage` (default 4.2V/cell).
- Scale vehicle speed to 0–200 km/h derived directly from instantaneous motor speed (`abs(motorSpeed) * 2`).
- Update `telemetry_Battery` and `telemetry_Speed` string buffers in `updateTelemetry()` every 250ms.
- Update UI design schema in `docs/radiokit-rc-ui-design.json` to assign `unit: "km/h"` for Speed.

**Non-Goals:**
- Closed-loop wheel encoder / GPS telemetry (this is an open-loop simulation).
- Changing battery cutoff shutdown voltage thresholds (cutoff remains enforced at `cutoff_voltage`).

## Decisions

### 1. Battery Percentage Warning-Floor Formula
* **Decision**: Compute battery percentage using `s_warningVoltage` ($V_{\text{warn}} = \text{battery.warningVoltage} \times \text{cellCount}$) as the 0% baseline:
  $$\text{pct} = \text{constrain}\left(\left\lfloor \frac{V_{\text{bat}} - V_{\text{warn}}}{V_{\text{full}} - V_{\text{warn}}} \times 100.0 \right\rfloor, 0, 100\right)$$
* **Rationale**: Drivers should be warned when the battery is depleted. Displaying 0% at the warning threshold provides clear, immediate feedback to recharge/swap the battery before the emergency motor cutoff engages.
* **Alternatives Considered**:
  - *Cutoff floor ($3.3V$)*: Leaves user with a false sense of remaining capacity when the battery is already in the danger zone.

### 2. Speed Telemetry Scaling (Option A: Direct Motor Speed)
* **Decision**: Speed in km/h is computed as `abs(motorSpeed) * 2`, mapping 0–100% motor duty to 0–200 km/h. In Park (P), `motorSpeed == 0` so speed correctly reads 0 km/h.
* **Rationale**: Instantaneous, clean, and directly reflects motor drive output without introducing complex transmission lag.
* **Alternatives Considered**:
  - *Flywheel RPM Inertia*: Introduces lag and may not reflect wheel speed accurately during wheel slip.

## Risks / Trade-offs

- **[Risk] Voltage Sag Under Heavy Motor Load**: Accelerating from a standstill can cause transient voltage sag that momentarily reduces battery percentage.
  - **Mitigation**: Battery voltage measurement in `HardwareInit::getBatteryVoltage()` utilizes low-pass filtering, and values below warning are smoothly clamped to 0%.
