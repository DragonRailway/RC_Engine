## Why

The current RC_brain control loop lacks safety mechanisms for battery protection (exposing LiPo batteries to over-discharge damage) and relies on manual bitmask toggling for lights without realistic automation. Additionally, the sound engine lacks explicit engine power state transitions (booting directly into idle) and physics-based trigger logic for Jake brake and Turbo blow-off sound effects.

Adding battery low-voltage cutoff, dynamic lighting automation, and physics-based engine sound FX will significantly enhance safety, realistic vehicle simulation, and user experience.

## What Changes

- **Battery Protection & Low Voltage Cutoff**: Automatically detect LiPo battery cell count (2S, 3S, 4S) at boot based on ADC voltage. Cut motor drive power, trigger an audible `outOfFuel` alarm, and flash hazard lights when voltage drops below 3.3V per cell.
- **Advanced Lighting Automation**: Implement automatic turn-signal cancellation when steering returns to center (|steer| < 10%), dynamic brake light activation on rapid throttle deceleration, 3-state headlight stepping (Off, Low Beam 40%, High Beam 100%), and synchronized hazard flashing.
- **Engine Start & Sound FX Logic**: Introduce a manual Engine Start/Stop state machine via RadioKit UI (booting in OFF state, requiring cranking sound sequence before enabling drive motor), and physics-based auto-triggers for Jake Brake (high-RPM deceleration) and Turbo Wastegate pop (sudden throttle drop).

## Capabilities

### New Capabilities
- `battery-protection`: Automatic LiPo cell detection, low-voltage threshold monitoring, motor cutoff, and alarm notifications.
- `advanced-lighting-automation`: Steering auto-cancel turn signals, rapid deceleration brake lights, headlight High/Low beam stepping, and hazard mode.
- `engine-start-sound-fx`: Engine Start/Stop state machine and physics-driven Jake Brake and Turbo Wastegate sound triggers.

### Modified Capabilities
- `vehicle-control-loop`: Update vehicle loop to evaluate battery safety, steering angle for turn signals, throttle decel for brake/FX, and engine power state.

## Impact

- **Affected Code**: `common/VehicleController.h`, `common/HardwareInit.h`, `src/RADIOKIT.h`, `lib/SoundEngine/src/RcEngineSound.h`, `src/main.cpp`.
- **APIs / Telemetry**: `telemetry_Battery` and `telemetry_Speed` in RadioKit.
- **Dependencies**: No external library additions required; leverages existing `ESP32_EasyKit`, `RcEngineSound`, and `RadioKit`.
