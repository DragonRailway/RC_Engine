# Lighting System

## 1. Overview

RC Brain supports multi-channel LED lighting with various effects. The system controls headlights, taillights, brake lights, turn signals, and more.

## 2. Hardware

### 2.1 LED Outputs

| Pin | Name | Function |
|-----|------|----------|
| 42 | L0 | Built-in LED (status indicator) |
| 6 | L1 | Light channel 1 |
| 7 | L2 | Light channel 2 |
| 8 | L3 | Light channel 3 |
| 9 | L4 | Light channel 4 |
| 10 | L5 | Light channel 5 |
| 11 | L6 | Light channel 6 |

### 2.2 PWM Configuration

- **Frequency**: 8 kHz (configurable via `LED_FREQ`)
- **Resolution**: 10-bit (0-1023) (configurable via `LED_RES`)

## 3. Light Types

### 3.1 Standard Lights

| Type | Function | Typical Use |
|------|----------|-------------|
| HEAD_LIGHT | Main headlights | Front illumination |
| TAIL_LIGHT | Rear lights | Rear illumination |
| BRAKE_LIGHT | Brake indicator | On when braking |
| REVERSING_LIGHT | Backup lights | On when reversed |
| HIGHBEAM_LIGHT | High beam | Extra front illumination |
| CABIN_LIGHT | Interior light | Cabin illumination |
| FOG_LIGHT | Fog lights | Low visibility |
| SIDE_LIGHT | Marker lights | Side visibility |
| ROOF_LIGHT | Roof lights | Top illumination |

### 3.2 Turn Signals

| Type | Function | Behavior |
|------|----------|----------|
| TURN_LIGHT | Turn signals | Left/Right indicators |
| HAZARD_LIGHT | Emergency flashers | Both turn signals |

### 3.3 Warning Lights

| Type | Function | Behavior |
|------|----------|----------|
| BEACON_LIGHT | Warning beacons | Double flash or rotate |

## 4. Configuration

### 4.1 Basic Light Config

```json
{
  "LIGHTS": {
    "HEAD_LIGHT": {
      "HARDWARE": "L1",            // Pin name
      "BRIGHTNESS_MAX": 60         // Brightness (0-100%)
    },
    "TAIL_LIGHT": {
      "HARDWARE": "L2",
      "BRIGHTNESS_MAX": 60
    },
    "BRAKE_LIGHT": {
      "HARDWARE": "L3"             // No brightness = full
    }
  }
}
```

### 4.2 Turn Signal Config

```json
{
  "LIGHTS": {
    "TURN_LIGHT": {
      "LEFT": {
        "HARDWARE": "L4"           // Left turn pin
      },
      "RIGHT": {
        "HARDWARE": "L5"           // Right turn pin
      },
      "BRIGHTNESS_MAX": 60,
      "TYPE": "BLINK",             // BLINK, FADE, SEQUENTIAL
      "INTERVAL_ON": 500,          // On duration (ms)
      "INTERVAL_OFF": 500          // Off duration (ms)
    }
  }
}
```

### 4.3 Hazard Light Config

```json
{
  "LIGHTS": {
    "HAZARD_LIGHT": {
      "HARDWARE": "TURN_LIGHT"     // References TURN_LIGHT
    }
  }
}
```

### 4.4 Reversing Light Config

```json
{
  "LIGHTS": {
    "REVERSING_LIGHT": {
      "HARDWARE": "BRAKE_LIGHT"    // References BRAKE_LIGHT
    }
  }
}
```

## 5. Light Effects

### 5.1 Effect Types

| Effect | Description | Use Case |
|--------|-------------|----------|
| BLINK | On/Off at defined interval | Turn signals, hazards |
| FADE | Smooth brightness transition | Soft on/off |
| SEQUENTIAL | Chasing light pattern | Sequential turn signals |
| XENON_LIGHTS | Startup flash effect | Xenon headlight simulation |

### 5.2 Timing Parameters

| Parameter | Description | Unit |
|-----------|-------------|------|
| INTERVAL_ON | Duration light is on | milliseconds |
| INTERVAL_OFF | Duration light is off | milliseconds |

## 6. Light Functions

### 6.1 Steady Lights

Always on when vehicle is powered:
- HEAD_LIGHT
- TAIL_LIGHT
- SIDE_LIGHT
- CABIN_LIGHT

### 6.2 Conditional Lights

On based on vehicle state:
- BRAKE_LIGHT - On when braking
- REVERSING_LIGHT - On when throttle is reversed
- TURN_LIGHT - On when steering is turned
- HAZARD_LIGHT - Manual toggle

### 6.3 Effect Lights

On with special patterns:
- BEACON_LIGHT - Double flash or rotating pattern
- TURN_LIGHT with BLINK/SEQUENTIAL effect

## 7. Brightness Control

### 7.1 Per-Light Brightness

Each light can have independent brightness:

```json
{
  "BRIGHTNESS_MAX": 60     // 0-100%
}
```

### 7.2 Global Volume

Sound volume affects overall system:

```json
{
  "SOUND": {
    "VOLUME": 80           // Master volume (0-100%)
  }
}
```

## 8. Hardware References

Lights can reference other light definitions to share pins:

```json
{
  "HAZARD_LIGHT": {
    "HARDWARE": "TURN_LIGHT"     // Uses same pins as TURN_LIGHT
  },
  "REVERSING_LIGHT": {
    "HARDWARE": "BRAKE_LIGHT"    // Uses same pin as BRAKE_LIGHT
  }
}
```

## 9. Special Features

### 9.1 Xenon Lights

When `XENON_LIGHTS` is defined, headlights show a bright flash on startup before settling to normal brightness.

### 9.2 Turn Signal Auto-Activate

Turn signals can automatically activate when steering is turned beyond a threshold:

```cpp
const uint16_t indicatorOn = 300;  // Threshold value
const boolean INDICATOR_DIR = true; // Direction adjustment
```