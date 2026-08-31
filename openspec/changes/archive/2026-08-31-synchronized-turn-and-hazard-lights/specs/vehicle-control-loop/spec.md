## ADDED Requirements

### Requirement: Synchronous Turn and Hazard Signal Control
The firmware SHALL control turn indicators and hazard warning lights through a unified synchronized interface (`HardwareInit::setTurnSignals`) ensuring:
1. When Hazard mode is active, both left and right turn signal LEDs flash in perfect synchronous phase (0 deg phase offset).
2. When transitioning from a single turn indicator (Left or Right) to Hazard mode, both blink timers SHALL be reset simultaneously in the same cycle.
3. The dashboard indicator audio click sound SHALL be synchronized with the active blink phase.

#### Scenario: Hazard lights activated while turn indicator is blinking
- **WHEN** the left turn signal is blinking and hazard warning is turned ON
- **THEN** both left and right indicators SHALL immediately synchronize and flash together in lockstep without phase offset.

#### Scenario: Hazard lights deactivated with no turn indicator
- **WHEN** hazard warning is turned OFF and no turn signals are active
- **THEN** both turn signal LEDs SHALL stop blinking and remain OFF.

#### Scenario: Turn indicator active without hazard
- **WHEN** left turn indicator is active and hazard is OFF
- **THEN** only the left turn LED SHALL blink while the right LED remains OFF.
