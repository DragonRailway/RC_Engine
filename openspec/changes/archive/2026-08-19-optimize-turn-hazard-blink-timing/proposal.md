## Why

Turn signal and hazard blinking in the firmware currently defaults to 500ms ON / 500ms OFF (1.0 Hz / 1000ms period). This creates two problems:
1. The flasher feels sluggish and slow to respond when toggled from the mobile control UI.
2. The flasher is out of sync with the vehicle sound engine's indicator click sample (~398ms duration), causing audio-visual phase drift.

## What Changes

- **Update Flasher Defaults**: Change default turn and hazard blink interval from 500ms ON / 500ms OFF to 300ms ON / 300ms OFF (1.67 Hz / 600ms period) across `common/Config.h`, `common/ConfigParser.h`, and board hardware configs (`hardware-*.json`).
- **Synchronize Audio Click Cadence**: Ensure the physical indicator flasher provides crisp, responsive 1.67 Hz cadence consistent with real automotive standards (ECE R48 / SAE J590).

## Capabilities

### Modified Capabilities
- `advanced-lighting-automation`: Update turn signal and hazard flasher interval requirement to 300ms ON / 300ms OFF.

## Impact

- **Firmware Files**: `common/Config.h`, `common/ConfigParser.h`, `configs/hardware_configs/hardware-*.json`.
- **Performance**: Snappy hazard and turn signal blinking aligned with audio feedback.
