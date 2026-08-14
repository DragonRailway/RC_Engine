## ADDED Requirements

### Requirement: Engine Start and Stop State Machine
The system SHALL boot in an engine OFF state and require a dedicated latched Engine Power toggle widget (`engine_start`, `RK_ToggleButton`) to start the engine: the engine transitions OFF → STARTING (cranking sound) when the toggle switches ON and RUNNING → OFF when the toggle switches OFF. Throttle input SHALL NOT start the engine; while the engine is OFF or STARTING, motor drive commands are disabled regardless of throttle widget position.

#### Scenario: Boot state is OFF
- **WHEN** system boots up and the Engine Power toggle is OFF
- **THEN** engine state is OFF, idle sound is silent, and motor drive commands are disabled

#### Scenario: Engine Start triggered
- **WHEN** the Engine Power toggle switches ON
- **THEN** engine state becomes STARTING, cranking audio sample plays, and upon completion transitions state to RUNNING

#### Scenario: Engine stop
- **WHEN** the Engine Power toggle switches OFF while the engine is RUNNING
- **THEN** engine state transitions to OFF and drive commands are disabled

#### Scenario: Start cancelled mid-crank
- **WHEN** the Engine Power toggle switches OFF while the engine is STARTING
- **THEN** the cranking start is cancelled and engine state returns to OFF

#### Scenario: Throttle does not auto-start
- **WHEN** throttle input is applied while the engine is OFF and the Engine Power toggle is OFF
- **THEN** the engine remains OFF and the drive motor stays disabled

#### Scenario: Reversing light independent of engine start
- **WHEN** the reversing light control bit (Item E / bit 4) is asserted
- **THEN** the reversing light turns on and the engine power state is unaffected

### Requirement: Physics-Based Jake Brake and Turbo Wastegate
The system SHALL evaluate engine RPM and throttle deceleration to automatically trigger Jake Brake compression sound and Turbo Wastegate blow-off sound, and SHALL increase the engine deceleration rate per the configured `jakebrake_decel_rate` while the Jake brake is engaged.

#### Scenario: Jake Brake auto-trigger
- **WHEN** throttle drops to zero while engine RPM is above 60% of maximum
- **THEN** Jake Brake compression sound effect triggers automatically

#### Scenario: Jake brake increases deceleration rate
- **WHEN** Jake brake is engaged while engine RPM is above the minimum threshold
- **THEN** the engine deceleration rate increases per the configured `jakebrake_decel_rate` and the Jake Brake voice plays

#### Scenario: Turbo Wastegate auto-trigger
- **WHEN** throttle drops rapidly from high revs
- **THEN** Turbo Wastegate blow-off pop sound effect triggers automatically

### Requirement: Engine RPM Inertia Ramp
The system SHALL simulate virtual flywheel mass by ramping engine RPM over time toward the throttle-commanded target, bounded by the configured acceleration (`acc`), deceleration (`dec`), and flywheel `inertia` parameters, so RPM does not jump instantly.

#### Scenario: Throttle acceleration ramp
- **WHEN** gas pedal is applied
- **THEN** current RPM ramps smoothly from idle toward the target RPM bounded by the `acc` parameter and flywheel `inertia`

#### Scenario: Throttle release deceleration
- **WHEN** gas pedal is released to zero
- **THEN** current RPM decays smoothly back to idle bounded by the `dec` parameter
