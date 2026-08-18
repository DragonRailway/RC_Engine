# Design: Mandatory Drivetrain Configuration

## Validation Flow

```
   ┌─────────────────────────────────────────────────────────┐
   │             Hardware Config JSON Staging                │
   └───────────────────────────┬─────────────────────────────┘
                               │
                               ▼
            Schema Validation (validate_configs.py)
            Requires top-level `"drivetrain"` key
                               │
                               ▼
                Firmware Boot (ConfigParser.h)
                               │
            ┌──────────────────┴──────────────────┐
            ▼                                     ▼
     Ackermann Mode                        Skid-Steer Mode
  Validates `driveMotor.configured`     Validates `leftMotor.configured` &
                                          `rightMotor.configured`
```
