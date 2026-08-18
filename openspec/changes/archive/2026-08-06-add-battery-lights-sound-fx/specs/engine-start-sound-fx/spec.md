## ADDED Requirements

### Requirement: Engine Start and Stop State Machine
The system SHALL boot in an engine OFF state, require an Engine Start trigger to transition through cranking sound (`STARTING`), and enable drive throttle only after transitioning to `RUNNING`.

#### Scenario: Boot state is OFF
- **WHEN** system boots up
- **THEN** engine state is OFF, idle sound is silent, and motor drive commands are disabled

#### Scenario: Engine Start triggered
- **WHEN** Engine Start button is pressed
- **THEN** engine state becomes STARTING, cranking audio sample plays, and upon completion transitions state to RUNNING

### Requirement: Physics-Based Jake Brake and Turbo Wastegate
The system SHALL evaluate engine RPM and throttle deceleration to automatically trigger Jake Brake compression sound and Turbo Wastegate blow-off sound.

#### Scenario: Jake Brake auto-trigger
- **WHEN** throttle drops to zero while engine RPM is above 60% of maximum
- **THEN** Jake Brake compression sound effect triggers automatically

#### Scenario: Turbo Wastegate auto-trigger
- **WHEN** throttle drops rapidly from high revs
- **THEN** Turbo Wastegate blow-off pop sound effect triggers automatically
