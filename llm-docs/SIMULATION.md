# Simulation

## 1. Overview

RC Brain simulates realistic engine behavior and generates corresponding sounds. The simulation drives RPM response, acceleration/deceleration curves, clutch engagement, transmission shifting, and multi-layered sound mixing.

---

## 2. Sound Engine

### 2.1 Architecture

The sound engine generates realistic vehicle sounds by mixing multiple audio layers in real-time. Each layer is an independent PCM sample triggered based on vehicle state (throttle, RPM, switches).

### 2.2 Sound Layers

| Layer | Description | Trigger |
|-------|-------------|---------|
| **Start** | Engine cranking/starting | Ignition |
| **Idle** | Base engine idle | Throttle = 0 |
| **Rev** | Engine revving under load | Throttle > REV_SWITCH_POINT |
| **Knock** | Diesel combustion pulses | RPM-dependent |
| **Turbo** | Turbocharger whistle | RPM-dependent |
| **Wastegate** | Blow-off valve release | Throttle drop with clutch engaged |
| **Jake Brake** | Compression brake | Throttle released + RPM > threshold |
| **Supercharger** | Supercharger whine | RPM-dependent |
| **Fan** | Cooling fan noise | RPM-dependent |

### 2.3 Layer Details

#### Start Sound
- Played once on ignition
- Volume controlled by `startVolumePercentage`

#### Idle Sound
- Plays when throttle = 0
- Crossfades with Rev sound as throttle increases
- Volume controlled by `idleVolumePercentage`
- Base engine layer mixed via `engineIdleVolumePercentage`

#### Rev Sound
- Plays when throttle > `revSwitchPoint`
- Replaces Idle sound completely at `idleEndPoint`
- Volume controlled by `revVolumePercentage`
- Base engine layer mixed via `engineRevVolumePercentage`

#### Knock Sound
- Short percussive pulses simulating diesel combustion
- `dieselKnockInterval` controls pulses per engine cycle
- Pattern varies by engine type:
  - **V8**: Pulses 4 and 8 louder (90° exhaust gap)
  - **V2**: First 2 of 4 pulses louder
  - **R6**: 6th pulse louder
- Volume controlled by `dieselKnockVolumePercentage`

#### Turbo Sound
- Continuous whistle that rises with RPM
- Mixed in parallel with engine sound
- Volume controlled by `turboVolumePercentage`

#### Wastegate Sound
- Triggered on rapid throttle drop with engaged clutch
- Simulates blow-off valve pressure release
- Volume controlled by `wastegateVolumePercentage`

#### Jake Brake Sound
- Plays when throttle released and RPM > `jakeBrakeMinRpm`
- Can slow engine in neutral (`JAKEBRAKE_ENGINE_SLOWDOWN`)
- Volume controlled by `jakeBrakeVolumePercentage`

### 2.4 Audio Parameters

- **Sample Rate**: 22,050 Hz
- **Bit Depth**: 8-bit PCM (signed, -128 to 127)
- **Channels**: Mono (summed for I2S output)
- **Volume Range**: 0-255% (allows boost beyond nominal)

### 2.5 Sound Files

Located in `lib/RcEngineSound/sounds/` (headers) and `src/sounds/` (JSON).

#### JSON Format
```json
{
  "sampleRate": 22050,
  "sampleCount": 4406,
  "samples": [0, 4, 14, 15, 16, 18, ...]
}
```

#### File Naming Convention
```
{type}-{vehicle}.json
```

Examples:
- `idle-ScaniaV8.json`
- `rev-ScaniaV8.json`
- `knock-ScaniaV8.json`
- `horn-ScaniaV8train.json`
- `airbrake-Truck2.json`
- `jakebrake-ScaniaV8.json`
- `turbo-ScaniaV8.json`
- `wastegate-1000HpScaniaV8.json`

### 2.6 Sound Conversion

```bash
./convert_sounds.sh
```

Input: `lib/RcEngineSound/src/vehicles/sounds/*.h`
Output: `lib/RcEngineSound/src/vehicles/sounds/json/*.json`

---

## 3. Engine Simulation

### 3.1 RPM Simulation

#### RPM Range
- **Idle RPM**: Base engine speed when throttle = 0
- **Max RPM**: Calculated as `IDLE_RPM × (MAX_RPM_PERCENTAGE / 100)`
- **Typical Values**:
  - Big Diesel trucks: 200% (2× idle)
  - Fast sports cars: 400% (4× idle)
  - Excavators: 200% (2× idle)

#### RPM Response
Engine RPM responds to throttle input with simulated inertia:

| Parameter | Description | Range |
|-----------|-------------|-------|
| `acc` | Acceleration step size | 1-9 |
| `dec` | Deceleration step size | 1-5 |

**Step Values**:
- `1` = Very slow (locomotive, heavy equipment)
- `2` = Slow (truck, excavator) - **default for automatic**
- `3` = Medium
- `5` = Fast (sports car)
- `9` = Very fast (trophy truck)

#### Super Slow Mode
For extremely heavy engines (locomotives, large excavators):
```cpp
#define SUPER_SLOW  // Enables ultra-slow RPM response
```

### 3.2 Acceleration & Deceleration

#### ESC Ramp Times
Controls how quickly the ESC responds to throttle changes:

| Parameter | Description | Typical |
|-----------|-------------|---------|
| `escRampTimeFirstGear` | Ramp time in 1st gear | 15-25 (20) |
| `escRampTimeSecondGear` | Ramp time in 2nd gear (used for auto) | 50-80 (50) |
| `escRampTimeThirdGear` | Ramp time in 3rd gear | 75 |
| `escBrakeSteps` | Braking deceleration rate | 20-30 (30) |
| `escAccelerationSteps` | Acceleration rate | 2-3 (3) |

#### Vehicle Type Profiles

| Vehicle Type | acc | dec | Ramp Time | Notes |
|--------------|-----|-----|-----------|-------|
| Locomotive | 1 | 1 | 100+ | Extremely slow response |
| Heavy Truck | 2 | 1 | 20-50 | Moderate response |
| Excavator | 2 | 1 | 20-50 | Moderate response |
| Sports Car | 5 | 3 | 10-20 | Quick response |
| Trophy Truck | 9 | 5 | 5-10 | Very quick response |

### 3.3 Clutch System

#### Clutch Engaging Point (CEP)
The clutch engages when engine RPM exceeds the CEP value:

| Parameter | Description | Range |
|-----------|-------------|-------|
| `clutchEngagingPoint` | RPM threshold for clutch engagement | 10-500 |

**Typical Values**:
- Automatic transmission: 10 (early engagement)
- Manual transmission: 90-100 (later engagement)
- Excavator: 500 (very late engagement)

#### Clutch Behavior
```
Throttle → RPM → Clutch State
─────────────────────────────
0%       → 0   → Disengaged
10%      → 50  → Disengaged
20%      → 100 → Engaging (at CEP)
30%      → 150 → Engaged
100%     → 500 → Fully Engaged
```

When disengaged:
- Engine sound plays freely
- Motor output is reduced or zero

When engaged:
- Engine sound syncs with motor speed
- Full motor power available

### 3.4 Transmission

#### Types
| Type | Description | Use Case |
|------|-------------|----------|
| NONE | Direct drive, linear RPM | Simple setups |
| MANUAL | Fixed gears, driver shifts | Real shifting transmissions |
| AUTOMATIC | Simulated torque converter | Most RC vehicles |

#### Automatic Transmission
Simulates a torque converter with gear ratios:

```json
{
  "TRANSMISSION": {
    "TYPE": "AUTOMATIC",
    "NUMBER_OF_GEARS": 3
  }
}
```

**Gear Ratios** (typical 3-speed):
- 1st: 1.0 (direct)
- 2nd: 0.7 (overdrive)
- 3rd: 0.5 (highway)

#### Gear Shifting Logic
- Upshift when RPM reaches ~80% of max
- Downshift when RPM drops to ~30% of max

#### Shifting Sound
When `shiftingAutoThrottle` is enabled:
- Throttle is temporarily reduced during shift
- Gear shift sound plays
- RPM drops to match next gear ratio

```cpp
const boolean shiftingAutoThrottle = true;
```

#### Double Clutch
For manual transmissions with double-clutching:
```cpp
const boolean doubleClutch = false;
```

**Note**: Cannot be used with automatic transmission.

---

## 4. Sound-RPM Integration

### 4.1 Idle-to-Rev Crossfade

Sound layers crossfade based on RPM:

```
RPM%:  0%────────30%────────50%────────100%
       │         │          │          │
Idle:  ████████████████████░░░░░░░░░░░░░░░░
Rev:   ░░░░░░░░░░░░░░░░░░░████████████████

Key: ████ = Active, ░░░░ = Inactive
```

| Parameter | Description |
|-----------|-------------|
| `revSwitchPoint` | RPM% where Rev starts mixing in |
| `idleEndPoint` | RPM% where Idle is fully replaced |

### 4.2 Knock Timing

Diesel knock pulses are timed to engine cycle:

```
V8 Engine Cycle (8 pulses per revolution):
─────────────────────────────────────────
Pulse:  1   2   3   4   5   6   7   8
Volume: 100% 50% 50% 100% 50% 50% 100% 100%
        ↑       ↑       ↑       ↑
        (Louder pulses due to exhaust grouping)
```

### 4.3 Turbo Response

Turbo whistle increases with RPM:

```
RPM%:  0%────────30%────────60%────────100%
       │         │          │          │
Turbo: ░░░░░░░░░░░████████████████████████
```

### 4.4 Wastegate Trigger

Wastegate sound triggers on:
1. Throttle is above 50%
2. Clutch is engaged
3. Throttle drops rapidly (>30% in <100ms)

---

## 5. Vehicle Type Configurations

### 5.1 Truck (Scania V8)

```json
{
  "ENGINE": {
    "ACCELERATION": 2,
    "DECELERATION": 1,
    "IDLE_RPM": 10,
    "CLUTCH_RPM": 100,
    "REV_SWITCH_POINT": 50,
    "IDLE_END_POINT": 40,
    "DIESEL_KNOCK_INTERVAL": 8,
    "DIESEL_KNOCK_START_POINT": 30,
    "JAKEBRAKE_MIN_RPM": 60
  },
  "TRANSMISSION": {
    "TYPE": "AUTOMATIC",
    "NUMBER_OF_GEARS": 3
  }
}
```

### 5.2 Excavator (Caterpillar 323)

```json
{
  "ENGINE": {
    "ACCELERATION": 2,
    "DECELERATION": 1,
    "IDLE_RPM": 10,
    "CLUTCH_RPM": 500,
    "REV_SWITCH_POINT": 100,
    "IDLE_END_POINT": 400,
    "DIESEL_KNOCK_INTERVAL": 6,
    "DIESEL_KNOCK_START_POINT": 110
  },
  "TRANSMISSION": {
    "TYPE": "NONE"
  }
}
```

### 5.3 Sports Car (Defender V8 Automatic)

```json
{
  "ENGINE": {
    "ACCELERATION": 2,
    "DECELERATION": 1,
    "CLUTCH_RPM": 10,
    "REV_SWITCH_POINT": 10,
    "IDLE_END_POINT": 500
  },
  "TRANSMISSION": {
    "TYPE": "AUTOMATIC",
    "NUMBER_OF_GEARS": 3
  }
}
```

---

## 6. Special Modes

### 6.1 Tracked Mode

For tanks, excavators, and tracked vehicles with dual throttle:

```cpp
#define TRACKED_MODE  // Dual throttle on CH2 and CH3
```

### 6.2 Dump Bed Mode

For vehicles with hydraulic dump bed:

```cpp
#define DUMP_BED  // Hydraulic dump bed control
```

---

## 7. Compile-Time Options

| Define | Description |
|--------|-------------|
| `SUPER_SLOW` | Ultra-slow RPM response for locomotives |
| `TRACKED_MODE` | Dual throttle for tracked vehicles |
| `DUMP_BED` | Hydraulic dump bed support |
| `JAKEBRAKE_ENGINE_SLOWDOWN` | Use jake brake to slow engine in neutral |
| `SEPARATE_FULL_BEAM` | Separate high beam control |
| `INDICATOR_SIDE_MARKERS` | Indicators double as side markers |
| `GEARBOX_WHINING` | Gearbox whining sound in neutral |
| `REV_SOUND` | Enable rev sound layer |
| `JAKE_BRAKE_SOUND` | Enable jake brake |
| `COUPLING_SOUND` | Enable coupling/uncoupling sounds |
| `V8` | V8 engine knock pattern |
| `V2` | V2 engine knock pattern |
| `R6` | R6 engine knock pattern |
| `RPM_DEPENDENT_KNOCK` | Knock volume varies with RPM |
| `XENON_LIGHTS` | Xenon bulb flash effect |

---

## 8. Parameter Reference

### 8.1 JSON Config Parameters

| Parameter | Description | Range | Default |
|-----------|-------------|-------|---------|
| `ACCELERATION` | Acceleration step | 1-9 | 2 |
| `DECELERATION` | Deceleration step | 1-5 | 1 |
| `IDLE_RPM` | Idle engine speed | 1-100 | 10 |
| `CLUTCH_RPM` | Clutch engagement point | 10-500 | 100 |
| `REV_SWITCH_POINT` | Rev sound mix start (%) | 0-100 | 50 |
| `IDLE_END_POINT` | Idle sound end (%) | 0-100 | 40 |
| `DIESEL_KNOCK_INTERVAL` | Pulses per cycle | 1-20 | 8 |
| `DIESEL_KNOCK_START_POINT` | Knock volume start (%) | 0-200 | 30 |
| `JAKEBRAKE_MIN_RPM` | Jake brake min RPM | 0-200 | 60 |
| `FAN_START_POINT` | Cooling fan start (%) | 0-100 | 0 |

### 8.2 Compile-Time Parameters

| Parameter | Description | Range | Default |
|-----------|-------------|-------|---------|
| `escRampTimeFirstGear` | 1st gear ramp time | 15-25 | 20 |
| `escRampTimeSecondGear` | 2nd gear ramp time | 50-80 | 50 |
| `escRampTimeThirdGear` | 3rd gear ramp time | 75-100 | 75 |
| `escBrakeSteps` | Braking deceleration | 20-30 | 30 |
| `escAccelerationSteps` | Acceleration rate | 2-3 | 3 |
| `clutchEngagingPoint` | Clutch RPM threshold | 10-500 | 90 |
| `MAX_RPM_PERCENTAGE` | Max RPM as % of idle | 200-400 | 330 |

### 8.3 Sound Volume Parameters

| Parameter | Description | Range |
|-----------|-------------|-------|
| `startVolumePercentage` | Start sound volume | 0-255% |
| `idleVolumePercentage` | Idle sound volume | 0-255% |
| `engineIdleVolumePercentage` | Base engine volume at idle | 0-100% |
| `fullThrottleVolumePercentage` | Max volume at full throttle | 0-255% |
| `revVolumePercentage` | Rev sound volume | 0-255% |
| `dieselKnockVolumePercentage` | Knock volume | 0-600% |
| `turboVolumePercentage` | Turbo volume | 0-255% |
| `wastegateVolumePercentage` | Wastegate volume | 0-255% |
| `jakeBrakeVolumePercentage` | Jake brake volume | 0-255% |
| `hornVolumePercentage` | Horn volume | 0-255% |
| `brakeVolumePercentage` | Air brake volume | 0-255% |

### 8.4 Pre-built Vehicle Profiles

Located in `lib/RcEngineSound/vehicles/` (80+ profiles):

- Scania V8, Scania 143
- Kenworth W900, Peterbilt
- Mercedes Actros, MAN TGX
- Volvo FH16
- Land Rover Defender V8
- Caterpillar 323 Excavator
- Tatra 813
- And many more...