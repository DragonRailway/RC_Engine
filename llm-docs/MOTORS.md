# Motor Control

## 1. Overview

RC Brain supports two motor control methods:
- **H-Bridge DC Motor**: Direct PWM control via onboard H-bridge drivers
- **ESC (Electronic Speed Controller)**: Servo signal (PPM) control

## 2. H-Bridge Control

### 2.1 Hardware

Two independent H-bridge channels on the TRACKLINK_V3 board:

| Channel | Pins | Mode |
|---------|------|------|
| **A** | PWM1=13, PWM2=14, BEMF=4 | Dual PWM (IN1/IN2) |
| **B** | DIR=15, PWM=16, BEMF=5 | DIR + PWM |
| **Common** | ENABLE=12 | Enable/disable both channels |

### 2.2 Pin Functions

| Pin | Function |
|-----|----------|
| PWM1 | Motor PWM channel 1 (H-Bridge A) |
| PWM2 | Motor PWM channel 2 (H-Bridge A) |
| DIR | Direction control (H-Bridge B) |
| PWM | Speed control (H-Bridge B) |
| BEMF | Back-EMF sensing (motor load detection) |
| COMMON_EN | H-Bridge enable (active high) |

### 2.3 PWM Configuration

- **Frequency**: 24 kHz (configurable via `DRV_FREQ`)
- **Resolution**: 8-bit (0-255) (configurable via `DRV_RES`)

## 3. ESC Control

### 3.1 Signal Protocol

- **Type**: PPM (Pulse Position Modulation)
- **Pulse Width**: 1000-2000 μs
- **Center**: 1500 μs (neutral)
- **Frequency**: 50 Hz (20ms period)

### 3.2 Pin Assignment

| Pin | Function |
|-----|----------|
| S1 (GPIO 1) | ESC signal output |
| S2 (GPIO 2) | ESC signal output |

## 4. Configuration

### 4.1 Hardware Config (`hardware-*.json`)

#### DC Motor (H-Bridge)
```json
{
  "DRIVE_MOTOR": {
    "HARDWARE": "HBRIDGE_A",       // HBRIDGE_A, HBRIDGE_B
    "FREQUENCY": 20000,            // PWM frequency (Hz)
    "DIRECTION": "FORWARD",        // FORWARD, REVERSE, UNI_FORWARD, UNI_REVERSE
    "DUTY": {
      "MIN": 20,                   // Minimum duty cycle (%)
      "MAX": 90                    // Maximum duty cycle (%)
    }
  }
}
```

#### ESC (Servo Signal)
```json
{
  "DRIVE_MOTOR": {
    "HARDWARE": "S1",              // S1 or S2
    "PROTOCOL": "PPM",             // PPM (DSHOT not implemented)
    "DIRECTION": "FORWARD",        // FORWARD, REVERSE, UNI_FORWARD, UNI_REVERSE
    "ENDPOINTS": {
      "MIN": 1000,                 // Minimum pulse width (μs)
      "MAX": 2000,                 // Maximum pulse width (μs)
      "CENTER": 1500,              // Center pulse width (μs)
      "DEADBAND": 10               // Deadband around center (μs)
    }
  }
}
```

### 4.2 Direction Modes

| Mode | Description |
|------|-------------|
| FORWARD | Normal forward/reverse |
| REVERSE | Inverted forward/reverse |
| UNI_FORWARD | Unidirectional forward only |
| UNI_REVERSE | Unidirectional reverse only |

## 5. Gearbox Control

### 5.1 Physical Gearbox

For vehicles with physical multi-speed transmissions (e.g., Tamiya 3-speed):

```json
{
  "GEARBOX": {
    "HARDWARE": "S2",              // Servo pin
    "ENDPOINTS": {
      "N": 1000,                   // Neutral (μs)
      "1": 1100,                   // Gear 1 (μs)
      "2": 1200,                   // Gear 2 (μs)
      "3": 1400,                   // Gear 3 (μs)
      "4": 1500,                   // Gear 4 (μs)
      "5": 1600                    // Gear 5 (μs)
    }
  }
}
```

### 5.2 Automatic Transmission Simulation

When no physical gearbox is present, the sound engine can simulate automatic shifting:

```json
{
  "TRANSMISSION": {
    "TYPE": "AUTOMATIC",           // AUTOMATIC, MANUAL, NONE
    "NUMBER_OF_GEARS": 3           // 3, 4, or 6
  }
}
```

## 6. BEMF Sensing

Back-EMF (Electromotive Force) sensing detects motor load:

- **Pin A**: GPIO 4
- **Pin B**: GPIO 5
- **Purpose**: Detect motor stalling, load changes, or speed

## 7. Servo Control

### 7.1 Steering Servo

```json
{
  "STEERING_SERVO": {
    "HARDWARE": "S1",              // Servo pin
    "FREQUENCY": 50,               // PWM frequency (Hz)
    "ENDPOINTS": {
      "LEFT": 1350,                // Left endpoint (μs)
      "RIGHT": 1650,               // Right endpoint (μs)
      "CENTER": 1500               // Center position (μs)
    }
  }
}
```

### 7.2 Servo Parameters

- **Frequency**: 50 Hz (20ms period)
- **Pulse Width**: 1000-2000 μs
- **Resolution**: 1 μs (1000 steps)