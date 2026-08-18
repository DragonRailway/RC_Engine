# Design: Unified Configured Flag

## Architecture

Standardizing `bool configured = false;` across all `HardwareConfig` structs ensures that every component follows the exact same interface pattern:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    UNIFIED CONFIGURATION CONTRACT                           │
└─────────────────────────────────────────────────────────────────────────────┘

  struct Sound         { ... bool configured = false; };
  struct DriveMotor    { ... bool configured = false; };
  struct SteeringServo { ... bool configured = false; };
  struct AuxMotor      { ... bool configured = false; };
  struct Light         { ... bool configured = false; };
  struct TurnLight     { ... bool configured = false; };
  struct DitchLight    { ... bool configured = false; };
  struct AuxLight      { ... bool configured = false; };
  struct Animation     { ... bool configured = false; };
  struct Battery       { ... bool configured = false; };
  struct Power         { ... bool configured = false; };
  struct Charging      { ... bool configured = false; };
```

`ConfigParser` sets `.configured = true` (or validates pin resolution for hardware channels) whenever a section is parsed from JSON.
